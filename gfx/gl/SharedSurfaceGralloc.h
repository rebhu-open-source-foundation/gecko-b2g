/* -*- Mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 40; -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef SHARED_SURFACE_GRALLOC_H_
#define SHARED_SURFACE_GRALLOC_H_

#include "mozilla/layers/CompositorTypes.h"
#include "mozilla/layers/LayersSurfaces.h"
#include "mozilla/UniquePtr.h"
#include "SharedSurface.h"

namespace mozilla {
namespace layers {
class GrallocTextureData;
class LayersIPCChannel;
}

namespace gl {
class GLContext;
class GLLibraryEGL;

class SharedSurface_Gralloc
    : public SharedSurface
{
public:
    static UniquePtr<SharedSurface_Gralloc> Create(GLContext* prodGL,
                                                   const GLFormats& formats,
                                                   const gfx::IntSize& size,
                                                   bool hasAlpha,
                                                   layers::TextureFlags flags,
                                                   layers::LayersIPCChannel* allocator);

    static SharedSurface_Gralloc* Cast(SharedSurface* surf) {
        MOZ_ASSERT(surf->mType == SharedSurfaceType::Gralloc);

        return (SharedSurface_Gralloc*)surf;
    }

protected:
    GLLibraryEGL* const mEGL;
    EGLSync mSync;
    RefPtr<layers::LayersIPCChannel> mAllocator;
    UniquePtr<layers::GrallocTextureData> mTextureData;
    const GLuint mProdTex;

    SharedSurface_Gralloc(GLContext* prodGL,
                          const gfx::IntSize& size,
                          bool hasAlpha,
                          GLLibraryEGL* egl,
                          layers::LayersIPCChannel* allocator,
                          UniquePtr<layers::GrallocTextureData> textureData,
                          GLuint prodTex);

    static bool HasExtensions(GLLibraryEGL* egl, GLContext* gl);

public:
    virtual ~SharedSurface_Gralloc();

    virtual void ProducerAcquireImpl() override {}
    virtual void ProducerReleaseImpl() override;

    virtual void WaitForBufferOwnership() override;

    virtual void LockProdImpl() override {}
    virtual void UnlockProdImpl() override {}

    virtual GLuint ProdTexture() override {
        return mProdTex;
    }

    virtual bool ToSurfaceDescriptor(layers::SurfaceDescriptor* const out_descriptor) override;

    virtual bool ReadbackBySharedHandle(gfx::DataSourceSurface* out_surface) override;

    virtual SharedSurface_Gralloc* AsSharedSurface_Gralloc() override { return this; }

    layers::GrallocTextureData* GetGrallocTextureData() {
        return mTextureData.get();
    }
};

class SurfaceFactory_Gralloc
    : public SurfaceFactory
{
public:
    SurfaceFactory_Gralloc(GLContext* prodGL, const SurfaceCaps& caps,
                           const RefPtr<layers::LayersIPCChannel>& allocator,
                           const layers::TextureFlags& flags);

    virtual UniquePtr<SharedSurface> CreateShared(const gfx::IntSize& size) override {
        bool hasAlpha = mReadCaps.alpha;

        UniquePtr<SharedSurface> ret;
        if (mAllocator) {
            ret = SharedSurface_Gralloc::Create(mGL, mFormats, size, hasAlpha,
                                                mFlags, mAllocator);
        }
        return ret;
    }
};

} /* namespace gl */
} /* namespace mozilla */

#endif /* SHARED_SURFACE_GRALLOC_H_ */
