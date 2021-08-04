/**
 * winspice - windows spice server
 *
 * Copyright 2019 Dunrong Huang <riegamaths@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "wspice.h"
#include <d3d11.h>
#include <dxgi1_2.h>
#include <stdio.h>
#include <stdlib.h>
#include "display.h"

static ID3D11Device *gDevice = NULL;
static ID3D11DeviceContext *gContext = NULL;
static IDXGIOutputDuplication *gOutputDuplication = NULL;
static ID3D11Texture2D *gAcquiredDesktopImage = NULL;
static IDXGISurface *surf = NULL; //获取这个dxgi_text2d的表面
static ID3D11Texture2D *sStage = NULL;

/**
 * initial process:
 * - Create D3D device(D3D11CreateDevice)
 * - Get DXGI device
 *     --> Get DXGI adapter
 *       --> Get output
 *         --> Get output1
 *           --> Create desktop duplication
 */

static int create_device(WinDisplay *wdisplay)
{
    HRESULT hr;
    D3D_FEATURE_LEVEL featureLevel;
    UINT driverTypeIndex;

    D3D_DRIVER_TYPE driverTypes[] = {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_1
    };
    /// Create D3D device
    for (driverTypeIndex = 0; driverTypeIndex < ARRAYSIZE(driverTypes); ++driverTypeIndex) {
        /* reference: https://docs.microsoft.com/en-us/windows/desktop/api/d3d11/nf-d3d11-d3d11createdevice */
        hr = D3D11CreateDevice(
            NULL,
            driverTypes[driverTypeIndex],
            NULL,
            0,
            featureLevels,
            ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION,
            &gDevice,
            &featureLevel,
            &gContext);
        if (SUCCEEDED(hr)) {
            break;
        }
    }
    if (FAILED(hr)) {
        printf("Failed to create device: %#lX\n", hr);
        return -1;
    }

    return 0;
}

static int get_duplication(WinDisplay *wdisplay)
{
    HRESULT hr;

    if (gAcquiredDesktopImage) {
        gAcquiredDesktopImage->lpVtbl->Release(gAcquiredDesktopImage);
        gAcquiredDesktopImage = NULL;
    }
    if (gOutputDuplication) {
        gOutputDuplication->lpVtbl->Release(gOutputDuplication);
        gOutputDuplication = NULL;
    }

    /// Get DXGI device
    IDXGIDevice *DxgiDevice = NULL;
    hr = gDevice->lpVtbl->QueryInterface(gDevice, &IID_IDXGIDevice, (void **)&DxgiDevice);
    if (FAILED(hr)) {
        printf("Failed to get dxgi device: %#lX\n", hr);
        return -1;
    }

    /// Get DXGI adapter
    IDXGIAdapter *DxgiAdapter = NULL;
    hr = DxgiDevice->lpVtbl->GetParent(DxgiDevice, &IID_IDXGIAdapter, (void**)&DxgiAdapter);
    DxgiDevice->lpVtbl->Release(DxgiDevice);
    DxgiDevice = NULL;
    if (FAILED(hr)) {
        printf("Failed to get dxgi adapter: %#lX\n", hr);
        return -1;
    }

    /// Get output
    DXGI_OUTPUT_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    IDXGIOutput* DxgiOutput = NULL;
    /* FIXME: other screenid? */
    int screenID = 0;

	hr = DxgiAdapter->lpVtbl->EnumOutputs(DxgiAdapter, screenID, &DxgiOutput);
	DxgiAdapter->lpVtbl->Release(DxgiAdapter);
	DxgiAdapter = NULL;
    if (FAILED(hr)) {
        printf("Failed to get output: %#lX\n", hr);
        return -1;
    }
    DXGI_OUTPUT_DESC* pDesc = &desc;
    hr = DxgiOutput->lpVtbl->GetDesc(DxgiOutput, pDesc);
    if (FAILED(hr)) {
        printf("Failed to get output desc: %#lX\n", hr);
        return -1;
    }

    /* FIXME: better way to get screen width? */
    if (pDesc->AttachedToDesktop) {
        RECT *pRect = &pDesc->DesktopCoordinates;
        wdisplay->width = pRect->right - pRect->left;
        wdisplay->height = pRect->bottom - pRect->top;
    } else {
        printf("FIXME: use better way to get output\n");
        return -1;
    }

    /// QI for Output 1
    IDXGIOutput1* DxgiOutput1 = NULL;
    hr = DxgiOutput->lpVtbl->QueryInterface(DxgiOutput, &IID_IDXGIOutput1, (void**)&DxgiOutput1);
    DxgiOutput->lpVtbl->Release(DxgiOutput);
    DxgiOutput = NULL;
    if (FAILED(hr)) {
        printf("Failed to get output1: %#lX\n", hr);
        return -1;
    }

    /// Create desktop duplication
    hr = DxgiOutput1->lpVtbl->DuplicateOutput(DxgiOutput1, (IUnknown*)gDevice, &gOutputDuplication);
    DxgiOutput1->lpVtbl->Release(DxgiOutput1);
    DxgiOutput1 = NULL;
    if (FAILED(hr)) {
        printf("Failed to get desktop duplication: %#lX\n", hr);
        return -1;
    }

    return 0;
}

static int win_display_init(WinDisplay *wdisplay)
{
    if (create_device(wdisplay) != 0) {
        return -1;
    }
    if (get_duplication(wdisplay) != 0) {
        return -1;
    }
    return 0;
}

static void release_update_frame(WinDisplay *wdisplay)
{
    if (gOutputDuplication) {
        HRESULT hr;
        /// refer: https://docs.microsoft.com/zh-cn/windows/desktop/api/dxgi1_2/nf-dxgi1_2-idxgioutputduplication-releaseframe
        hr = gOutputDuplication->lpVtbl->ReleaseFrame(gOutputDuplication);
        if (hr != S_OK) {
            if (hr == DXGI_ERROR_ACCESS_LOST) {
                printf("DXGI lost, recreate it\n");
            } else {
                printf("Failed to release frame: %#lX\n", hr);
            }
        }
    }
    if (gAcquiredDesktopImage) {
        gAcquiredDesktopImage->lpVtbl->Release(gAcquiredDesktopImage);
        gAcquiredDesktopImage = NULL;
    }
}

static void release_screen_bitmap(WinDisplay *wdisplay)
{
    if (surf) {
        surf->lpVtbl->Unmap(surf);
        surf->lpVtbl->Release(surf);
        surf = NULL;
    }
    if (sStage) {
        sStage->lpVtbl->Release(sStage);
        sStage = NULL;
    }

    wdisplay->accumulated_frames = 0;
    wdisplay->total_metadata_buffer_size = 0;
}

static int update_changes(WinDisplay *wdisplay)
{
    HRESULT hr = S_OK;
    IDXGIResource* DesktopResource = 0;
    DXGI_OUTDUPL_FRAME_INFO FrameInfo;

    if (wdisplay->accumulated_frames > 0) {
        release_screen_bitmap(wdisplay);
    }

    /// 截取屏幕数据，但是还不能直接访问原始数据
    hr = gOutputDuplication->lpVtbl->AcquireNextFrame(
        gOutputDuplication, 500, &FrameInfo, &DesktopResource);
    if (hr != S_OK) {
        /// refer: https://docs.microsoft.com/zh-cn/windows/desktop/api/dxgi1_2/nf-dxgi1_2-idxgioutputduplication-acquirenextframe
        if (hr == DXGI_ERROR_ACCESS_LOST) {
            printf("DXGI lost, recreate it\n");
			get_duplication(wdisplay);
			return -1;
        } else if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
            return -1;
        } else {
            printf("Failed to get next frame: %#lX\n", hr);
            return -1;
        }
    }
    /// 获取纹理2D
    hr = DesktopResource->lpVtbl->QueryInterface(DesktopResource, &IID_ID3D11Texture2D, (void **)&gAcquiredDesktopImage);
    DesktopResource->lpVtbl->Release(DesktopResource);
    DesktopResource = NULL;
    if (FAILED(hr)) {
        printf("Failed to QI for DesktopResource\n");
        release_update_frame(wdisplay);
        return -1;
    }
    wdisplay->accumulated_frames = FrameInfo.AccumulatedFrames;
    wdisplay->total_metadata_buffer_size = FrameInfo.TotalMetadataBufferSize;
    memcpy(&wdisplay->FrameInfo, &FrameInfo, sizeof(FrameInfo));

    return 0;
}

static bool display_have_updates(WinDisplay *wdisplay)
{
	if (wdisplay->accumulated_frames == 0)
		return false;

    return true;
}

static int find_invalid_region(WinDisplay *wdisplay)
{
    // 获取 dirty 区域
    HRESULT hr;
    UINT i;
    BYTE *dataBuffer = NULL;
    UINT bufSize;
    BYTE *dirtyRects;
    UINT dirtyRectSize;
    RECT *invalid;
    RECT *pRect;

    if (wdisplay->accumulated_frames == 0 || wdisplay->total_metadata_buffer_size == 0) {
        printf("No accumulated frames\n");
        return -1;
    }

    dataBuffer = (BYTE *)malloc(wdisplay->total_metadata_buffer_size);
    if (!dataBuffer) {
        printf("Failed to allocate memory for metadata");
        exit(1);
    }

    bufSize = wdisplay->total_metadata_buffer_size;
    hr = gOutputDuplication->lpVtbl->GetFrameMoveRects(gOutputDuplication, bufSize, (DXGI_OUTDUPL_MOVE_RECT *)dataBuffer, &bufSize);
    if (FAILED(hr)) {
        printf("Failed to get frame move rects");
        goto failed;
    }

    dirtyRects = dataBuffer + bufSize;
    bufSize = wdisplay->total_metadata_buffer_size - bufSize;

    hr = gOutputDuplication->lpVtbl->GetFrameDirtyRects(gOutputDuplication, bufSize, (RECT *)dirtyRects, &bufSize);
    if (FAILED(hr)) {
        printf("Failed to get frame dirty rects");
        goto failed;
    }
    dirtyRectSize = bufSize / sizeof(RECT);
    pRect = (RECT *)dirtyRects;
    invalid = &wdisplay->invalid;
    for (i = 0; i < dirtyRectSize; ++i) {
        UnionRect(invalid, invalid, pRect);
        ++pRect;
    }
    free(dataBuffer);
    return 0;

failed:
    if (dataBuffer) {
        free(dataBuffer);
    }
    return -1;
}

static void clear_invalid_region(WinDisplay *wdisplay)
{
    SetRectEmpty(&wdisplay->invalid);
}

static int get_screen_bitmap(WinDisplay *wdisplay, uint8_t **bitmap, int *pitch)
{
    if (!bitmap || !pitch) {
        return -1;
    }
    HRESULT hr;
    D3D11_TEXTURE2D_DESC tDesc;
    D3D11_BOX box;
    RECT *invalid = &wdisplay->invalid;

    tDesc.Width = (invalid->right - invalid->left);
    tDesc.Height = (invalid->bottom - invalid->top);
    tDesc.MipLevels = 1;
    tDesc.ArraySize = 1;
    tDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    tDesc.SampleDesc.Count = 1;
    tDesc.SampleDesc.Quality = 0;
    tDesc.Usage = D3D11_USAGE_STAGING;
    tDesc.BindFlags = 0;
    tDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    tDesc.MiscFlags = 0;

    box.top = invalid->top;
    box.left = invalid->left;
    box.right = invalid->right;
    box.bottom = invalid->bottom;
    box.front = 0;
    box.back = 1;

    /// 创建一个新的2D纹理对象，用于把 gAcquiredDesktopImage的数据copy进去
    hr = gDevice->lpVtbl->CreateTexture2D(gDevice, &tDesc, NULL, &sStage);
    if (FAILED(hr)) {
        printf("Failed to CreateTexture2D: %#lX\n", hr);
        return -1;
    }

    gContext->lpVtbl->CopySubresourceRegion(gContext, (ID3D11Resource*)sStage, 0,0,0,0, (ID3D11Resource*) gAcquiredDesktopImage, 0, &box);

    hr = sStage->lpVtbl->QueryInterface(sStage, &IID_IDXGISurface, (void **)(&surf));
    if (FAILED(hr)) {
        printf("Failed to QI staging surface: %#lX\n", hr);
        return -1;
    }

    /// copy bits to user space
    DXGI_MAPPED_RECT mappedRect;
    hr = surf->lpVtbl->Map(surf, &mappedRect, DXGI_MAP_READ);

    if (SUCCEEDED(hr)) {
        *bitmap = (uint8_t *)malloc(mappedRect.Pitch * tDesc.Height);
        memcpy(*bitmap, mappedRect.pBits, mappedRect.Pitch * tDesc.Height);
        *pitch = mappedRect.Pitch;
    }

    return 0;
}

#define BPP         4
static int ProcessMonoMask(WinDisplay *wdisplay, bool IsMono, PTR_INFO* PtrInfo, INT* PtrWidth, INT* PtrHeight, INT* PtrLeft, INT* PtrTop, BYTE** InitBuffer, D3D11_BOX* Box)
{
    // Desktop dimensions
    INT DesktopWidth = wdisplay->width;
    INT DesktopHeight = wdisplay->height;

    // Pointer position
    INT GivenLeft = PtrInfo->Position.x;
    INT GivenTop = PtrInfo->Position.y;

    // Figure out if any adjustment is needed for out of bound positions
    if (GivenLeft < 0) {
        *PtrWidth = GivenLeft + (INT)(PtrInfo->ShapeInfo.Width);
    } else if ((GivenLeft + (INT)(PtrInfo->ShapeInfo.Width)) > DesktopWidth) {
        *PtrWidth = DesktopWidth - GivenLeft;
    } else {
        *PtrWidth = (INT)(PtrInfo->ShapeInfo.Width);
    }

    if (IsMono) {
        PtrInfo->ShapeInfo.Height = PtrInfo->ShapeInfo.Height / 2;
    }

    if (GivenTop < 0) {
        *PtrHeight = GivenTop + (INT)(PtrInfo->ShapeInfo.Height);
    } else if ((GivenTop + (INT)(PtrInfo->ShapeInfo.Height)) > DesktopHeight) {
        *PtrHeight = DesktopHeight - GivenTop;
    } else {
        *PtrHeight = (INT)(PtrInfo->ShapeInfo.Height);
    }

    if (IsMono) {
        PtrInfo->ShapeInfo.Height = PtrInfo->ShapeInfo.Height * 2;
    }

    *PtrLeft = (GivenLeft < 0) ? 0 : GivenLeft;
    *PtrTop = (GivenTop < 0) ? 0 : GivenTop;

    // Staging buffer/texture
    D3D11_TEXTURE2D_DESC CopyBufferDesc;
    CopyBufferDesc.Width = *PtrWidth;
    CopyBufferDesc.Height = *PtrHeight;
    CopyBufferDesc.MipLevels = 1;
    CopyBufferDesc.ArraySize = 1;
    CopyBufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    CopyBufferDesc.SampleDesc.Count = 1;
    CopyBufferDesc.SampleDesc.Quality = 0;
    CopyBufferDesc.Usage = D3D11_USAGE_STAGING;
    CopyBufferDesc.BindFlags = 0;
    CopyBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    CopyBufferDesc.MiscFlags = 0;

    ID3D11Texture2D* CopyBuffer = NULL;
    HRESULT hr = gDevice->lpVtbl->CreateTexture2D(gDevice, &CopyBufferDesc, NULL, &CopyBuffer);
    if (FAILED(hr)) {
        printf("Failed creating staging texture for pointer: %#lX\n", hr);
        return -1;
    }

    // Copy needed part of desktop image
    Box->left = *PtrLeft;
    Box->top = *PtrTop;
    Box->right = *PtrLeft + *PtrWidth;
    Box->bottom = *PtrTop + *PtrHeight;
    gContext->lpVtbl->CopySubresourceRegion(gContext, (ID3D11Resource*)CopyBuffer, 0, 0, 0, 0, (ID3D11Resource*) gAcquiredDesktopImage, 0, Box);

    // QI for IDXGISurface
    IDXGISurface* CopySurface = NULL;
    hr = CopyBuffer->lpVtbl->QueryInterface(CopyBuffer, &IID_IDXGISurface, (void **)&CopySurface);
    CopyBuffer->lpVtbl->Release(CopyBuffer);
    CopyBuffer = NULL;
    if (FAILED(hr)) {
        printf("Failed to QI staging texture into IDXGISurface for pointer: %#lX\n", hr);
        return -1;
    }

    // Map pixels
    DXGI_MAPPED_RECT MappedSurface;
    hr = CopySurface->lpVtbl->Map(CopySurface, &MappedSurface, DXGI_MAP_READ);
    if (FAILED(hr)) {
        CopySurface->lpVtbl->Release(CopySurface);
        CopySurface = NULL;
        printf("Failed to map surface for pointer: %#lX\n", hr);
        return -1;
    }

    // New mouseshape buffer
    *InitBuffer = (BYTE *)malloc(*PtrWidth * *PtrHeight * BPP);
    memset(*InitBuffer, 0, *PtrWidth * *PtrHeight * BPP);

    UINT* InitBuffer32 = (UINT *)(*InitBuffer);
    UINT* Desktop32 = (UINT *)(MappedSurface.pBits);
    UINT  DesktopPitchInPixels = MappedSurface.Pitch / sizeof(UINT);

    // What to skip (pixel offset)
    UINT SkipX = (GivenLeft < 0) ? (-1 * GivenLeft) : (0);
    UINT SkipY = (GivenTop < 0) ? (-1 * GivenTop) : (0);

    if (IsMono) {
        for (INT Row = 0; Row < *PtrHeight; ++Row) {
            // Set mask
            BYTE Mask = 0x80;
            Mask = Mask >> (SkipX % 8);
            for (INT Col = 0; Col < *PtrWidth; ++Col) {
                // Get masks using appropriate offsets
                BYTE AndMask = PtrInfo->PtrShapeBuffer[((Col + SkipX) / 8) + ((Row + SkipY) * (PtrInfo->ShapeInfo.Pitch))] & Mask;
                BYTE XorMask = PtrInfo->PtrShapeBuffer[((Col + SkipX) / 8) + ((Row + SkipY + (PtrInfo->ShapeInfo.Height / 2)) * (PtrInfo->ShapeInfo.Pitch))] & Mask;
                UINT AndMask32 = (AndMask) ? 0xFFFFFFFF : 0xFF000000;
                UINT XorMask32 = (XorMask) ? 0x00FFFFFF : 0x00000000;

                // Set new pixel
                InitBuffer32[(Row * *PtrWidth) + Col] = (Desktop32[(Row * DesktopPitchInPixels) + Col] & AndMask32) ^ XorMask32;

                // Adjust mask
                if (Mask == 0x01) {
                    Mask = 0x80;
                } else {
                    Mask = Mask >> 1;
                }
            }
        }
    } else {
        UINT* Buffer32 = (UINT *)(PtrInfo->PtrShapeBuffer);

        // Iterate through pixels
        for (INT Row = 0; Row < *PtrHeight; ++Row) {
            for (INT Col = 0; Col < *PtrWidth; ++Col) {
                // Set up mask
                UINT MaskVal = 0xFF000000 & Buffer32[(Col + SkipX) + ((Row + SkipY) * (PtrInfo->ShapeInfo.Pitch / sizeof(UINT)))];
                if (MaskVal) {
                    // Mask was 0xFF
                    InitBuffer32[(Row * *PtrWidth) + Col] = (Desktop32[(Row * DesktopPitchInPixels) + Col] ^ Buffer32[(Col + SkipX) + ((Row + SkipY) * (PtrInfo->ShapeInfo.Pitch / sizeof(UINT)))]) | 0xFF000000;
                } else {
                    // Mask was 0x00
                    InitBuffer32[(Row * *PtrWidth) + Col] = Buffer32[(Col + SkipX) + ((Row + SkipY) * (PtrInfo->ShapeInfo.Pitch / sizeof(UINT)))] | 0xFF000000;
                }
            }
        }
    }

    // Done with resource
    hr = CopySurface->lpVtbl->Unmap(CopySurface);
    CopySurface->lpVtbl->Release(CopySurface);

    CopySurface = NULL;
    if (FAILED(hr)) {
        printf("Failed to unmap surface for pointer: %#lX\n", hr);
        return -1;
    }

    return 0;
}

static bool mouse_have_updates(WinDisplay *wdisplay)
{
    DXGI_OUTDUPL_FRAME_INFO *FrameInfo = &wdisplay->FrameInfo;
    PTR_INFO *PtrInfo = wdisplay->PtrInfo; /* FIXME: use struct */
    bool mouse_changed = false;

    // A non-zero mouse update timestamp indicates that there is a mouse position update and optionally a shape change
    if (FrameInfo->LastMouseUpdateTime.QuadPart != 0 &&
        FrameInfo->LastMouseUpdateTime.QuadPart > PtrInfo->LastTimeStamp.QuadPart) {
        mouse_changed = true;
    }
    PtrInfo->Position.x = FrameInfo->PointerPosition.Position.x;
    PtrInfo->Position.y = FrameInfo->PointerPosition.Position.y;
    PtrInfo->LastTimeStamp = FrameInfo->LastMouseUpdateTime;
    PtrInfo->Visible = FrameInfo->PointerPosition.Visible;

    return mouse_changed;
}

static bool mouse_have_new_shape(WinDisplay *wdisplay)
{
    DXGI_OUTDUPL_FRAME_INFO *FrameInfo = &wdisplay->FrameInfo;

    bool mouse_shape_changed = false;

    if (FrameInfo->PointerShapeBufferSize > 0) {
        mouse_shape_changed = true;
    }

    return mouse_shape_changed;
}

static int mouse_get_new_shape(WinDisplay *wdisplay, WinSpiceCursor **cursor)
{
    DXGI_OUTDUPL_FRAME_INFO *FrameInfo = &wdisplay->FrameInfo;
    PTR_INFO *PtrInfo = wdisplay->PtrInfo; /* FIXME: use struct */
    UINT BufferSizeRequired;
    WinSpiceCursor *c = NULL;

    INT PtrWidth  = 0;
    INT PtrHeight = 0;
    INT PtrLeft   = 0;
    INT PtrTop    = 0;
    D3D11_BOX Box;
    Box.front = 0;
    Box.back  = 1;

    /* FIXME: memery cache? */
    PtrInfo->PtrShapeBuffer = malloc(FrameInfo->PointerShapeBufferSize);

    // Get shape
    HRESULT hr = gOutputDuplication->lpVtbl->GetFramePointerShape(
        gOutputDuplication,
        FrameInfo->PointerShapeBufferSize,
        (void *)(PtrInfo->PtrShapeBuffer),
        &BufferSizeRequired,
        &(PtrInfo->ShapeInfo));
    if (FAILED(hr)) {
        printf("Failed to get mouse: %#lX\n", hr);
        return -1;
    }

    switch (PtrInfo->ShapeInfo.Type) {
    case DXGI_OUTDUPL_POINTER_SHAPE_TYPE_COLOR: {
        PtrWidth = PtrInfo->ShapeInfo.Width;
        PtrHeight = PtrInfo->ShapeInfo.Height;
        *cursor = malloc(sizeof(WinSpiceCursor) + PtrInfo->ShapeInfo.Pitch * PtrHeight);
        memcpy((*cursor)->data, PtrInfo->PtrShapeBuffer,  PtrInfo->ShapeInfo.Pitch * PtrHeight);
        break;
    }
    case DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME: {
        /// monochrome documentation reference: http://www.fastgraph.com/help/defining_the_mouse_cursor.html
        PtrWidth = PtrInfo->ShapeInfo.Width;
        PtrHeight = PtrInfo->ShapeInfo.Height / 2;
        int bpl = (PtrWidth + 7) / 8;
        *cursor = malloc(sizeof(WinSpiceCursor) + bpl * PtrInfo->ShapeInfo.Height);
        memcpy((*cursor)->data, PtrInfo->PtrShapeBuffer, bpl * PtrInfo->ShapeInfo.Height);
        break;
    }
    case DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MASKED_COLOR: {
        /* FIXME: fix later */
        BYTE* InitBuffer = NULL;
        printf("FIXME! UNIMPLEMENTED! %s\n", __func__);
        ProcessMonoMask(wdisplay, false, PtrInfo, &PtrWidth, &PtrHeight, &PtrLeft, &PtrTop, &InitBuffer, &Box);
        *cursor = malloc(sizeof(WinSpiceCursor) + PtrWidth * BPP * PtrHeight);
        memcpy((*cursor)->data, InitBuffer, PtrWidth * BPP * PtrHeight);
        free(InitBuffer);
        break;
    }
    default:
        break;
    }

    c = *cursor;
    c->width = PtrWidth;
    c->height = PtrHeight;
    c->hot_x = PtrInfo->ShapeInfo.HotSpot.x;
    c->hot_y = PtrInfo->ShapeInfo.HotSpot.y;
    c->ptr_type = PtrInfo->ShapeInfo.Type;
    free(PtrInfo->PtrShapeBuffer);

    return 0;
}

WinDisplay *win_display_new(GAsyncQueue *drawable_queue)
{
    WinDisplay *wdisplay = (WinDisplay *)malloc(sizeof(WinDisplay));
    if (!wdisplay) {
        return NULL;
    }
    ZeroMemory(wdisplay, sizeof(*wdisplay));

    SetRectEmpty(&wdisplay->invalid);

    wdisplay->drawable_queue = drawable_queue;

    wdisplay->update_changes = update_changes;
    wdisplay->release_update_frame = release_update_frame;
    wdisplay->display_have_updates = display_have_updates;
    wdisplay->find_invalid_region = find_invalid_region;
    wdisplay->clear_invalid_region = clear_invalid_region;
    wdisplay->get_screen_bitmap = get_screen_bitmap;
    wdisplay->PtrInfo = malloc(sizeof(PTR_INFO));

    /// mouse
    wdisplay->mouse_have_updates = mouse_have_updates;
    wdisplay->mouse_have_new_shape = mouse_have_new_shape;
    wdisplay->mouse_get_new_shape = mouse_get_new_shape;

    if (win_display_init(wdisplay) != 0) {
        printf("Failed to init win_display\n");
        goto failed;
    }

    return wdisplay;

failed:
    win_display_free(wdisplay);
    return NULL;
}

void win_display_free(WinDisplay *wdisplay)
{
    if (!wdisplay) {
        return;
    }
    free(wdisplay);
    printf("FIXME! %s UNIMPLEMENTED!\n", __FUNCTION__);
}
