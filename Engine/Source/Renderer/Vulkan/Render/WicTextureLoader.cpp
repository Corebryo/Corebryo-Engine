/*
 * Corebryo
 * Copyright (c) 2026 Jonathan Den Haerynck
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "WicTextureLoader.h"

#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <cstdio>
#include <string>

namespace
{
    /* Convert UTF-8 or ANSI path to wide string. */
    bool ToWidePath(const char* Path, std::wstring& OutWide)
    {
        if (!Path || Path[0] == '\0')
        {
            return false;
        }

        UINT codePage = CP_UTF8;
        int wideSize =
            MultiByteToWideChar(codePage, 0, Path, -1, nullptr, 0);

        if (wideSize == 0)
        {
            codePage = CP_ACP;
            wideSize =
                MultiByteToWideChar(codePage, 0, Path, -1, nullptr, 0);
        }

        if (wideSize == 0)
        {
            return false;
        }

        OutWide.resize(static_cast<size_t>(wideSize));

        const int converted =
            MultiByteToWideChar(
                codePage, 0, Path, -1, OutWide.data(), wideSize);

        if (converted == 0)
        {
            return false;
        }

        OutWide.resize(static_cast<size_t>(converted - 1));
        return true;
    }
}

/* Load PNG image into raw RGBA texture data. */
bool LoadPngWic(const char* Path, TextureData& OutData)
{
    /* Reset output data. */
    OutData = TextureData();

    /* Initialize COM for WIC usage. */
    const HRESULT initResult =
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    const bool didInit = SUCCEEDED(initResult);
    if (FAILED(initResult) &&
        initResult != RPC_E_CHANGED_MODE)
    {
        std::fprintf(stderr,
            "LoadPngWic: CoInitializeEx failed\n");
        return false;
    }

    /* Create WIC imaging factory. */
    Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
    HRESULT hr =
        CoCreateInstance(
            CLSID_WICImagingFactory2,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&factory));

    if (FAILED(hr))
    {
        hr =
            CoCreateInstance(
                CLSID_WICImagingFactory,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&factory));
    }

    if (FAILED(hr))
    {
        std::fprintf(stderr,
            "LoadPngWic: Failed to create WIC factory\n");

        if (didInit && initResult != RPC_E_CHANGED_MODE)
        {
            CoUninitialize();
        }
        return false;
    }

    /* Convert file path to wide string. */
    std::wstring widePath;
    if (!ToWidePath(Path, widePath))
    {
        std::fprintf(stderr,
            "LoadPngWic: Failed to convert path\n");

        if (didInit && initResult != RPC_E_CHANGED_MODE)
        {
            CoUninitialize();
        }
        return false;
    }

    /* Create bitmap decoder. */
    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    hr =
        factory->CreateDecoderFromFilename(
            widePath.c_str(),
            nullptr,
            GENERIC_READ,
            WICDecodeMetadataCacheOnLoad,
            &decoder);

    if (FAILED(hr))
    {
        std::fprintf(stderr,
            "LoadPngWic: Failed to decode %s\n", Path);

        if (didInit && initResult != RPC_E_CHANGED_MODE)
        {
            CoUninitialize();
        }
        return false;
    }

    /* Retrieve first image frame. */
    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, &frame);

    if (FAILED(hr))
    {
        std::fprintf(stderr,
            "LoadPngWic: Failed to get PNG frame\n");

        if (didInit && initResult != RPC_E_CHANGED_MODE)
        {
            CoUninitialize();
        }
        return false;
    }

    /* Query image dimensions. */
    UINT width = 0;
    UINT height = 0;
    frame->GetSize(&width, &height);

    if (width == 0 || height == 0)
    {
        std::fprintf(stderr,
            "LoadPngWic: Invalid PNG size\n");

        if (didInit && initResult != RPC_E_CHANGED_MODE)
        {
            CoUninitialize();
        }
        return false;
    }

    /* Create format converter. */
    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    hr = factory->CreateFormatConverter(&converter);

    if (FAILED(hr))
    {
        std::fprintf(stderr,
            "LoadPngWic: Failed to create format converter\n");

        if (didInit && initResult != RPC_E_CHANGED_MODE)
        {
            CoUninitialize();
        }
        return false;
    }

    /* Convert image to RGBA format. */
    hr =
        converter->Initialize(
            frame.Get(),
            GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0,
            WICBitmapPaletteTypeCustom);

    if (FAILED(hr))
    {
        std::fprintf(stderr,
            "LoadPngWic: Failed to convert to RGBA\n");

        if (didInit && initResult != RPC_E_CHANGED_MODE)
        {
            CoUninitialize();
        }
        return false;
    }

    /* Allocate pixel buffer. */
    OutData.Width = width;
    OutData.Height = height;
    OutData.Pixels.resize(
        static_cast<size_t>(width) * height * 4);

    /* Copy pixel data. */
    hr =
        converter->CopyPixels(
            nullptr,
            width * 4,
            static_cast<UINT>(OutData.Pixels.size()),
            OutData.Pixels.data());

    if (FAILED(hr))
    {
        std::fprintf(stderr,
            "LoadPngWic: CopyPixels failed\n");

        OutData = TextureData();

        if (didInit && initResult != RPC_E_CHANGED_MODE)
        {
            CoUninitialize();
        }
        return false;
    }

    /* Shutdown COM if owned. */
    if (didInit && initResult != RPC_E_CHANGED_MODE)
    {
        CoUninitialize();
    }

    return true;
}
