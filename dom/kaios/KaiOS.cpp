/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */

#include "mozilla/dom/KaiOS.h"
#include "mozilla/dom/KaiOSBinding.h"

namespace mozilla {
namespace dom {

KaiOS::KaiOS(nsPIDOMWindowInner* aWindow) : mWindow(aWindow) {}
KaiOS::~KaiOS() {}

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(KaiOS)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(KaiOS)
NS_IMPL_CYCLE_COLLECTING_RELEASE(KaiOS)

NS_IMPL_CYCLE_COLLECTION_CLASS(KaiOS)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(KaiOS)
  tmp->Invalidate();
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mWindow)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(KaiOS)
#ifdef MOZ_B2G_RIL
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mIccManager)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mVoicemail)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mMobileConnections)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mTelephony)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDataCallManager)
#endif
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(KaiOS)

void KaiOS::Invalidate() {

#ifdef MOZ_B2G_RIL
  if (mIccManager) {
    mIccManager = nullptr;
  }

  if (mMobileConnections) {
    mMobileConnections = nullptr;
  }

  if (mTelephony) {
    mTelephony = nullptr;
  }

  if (mDataCallManager) {
    mDataCallManager = nullptr;
  }

  if (mVoicemail) {
    mVoicemail = nullptr;
  }
#endif
}

size_t KaiOS::SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const {
  size_t n = aMallocSizeOf(this);

  return n;
}

JSObject* KaiOS::WrapObject(JSContext* cx, JS::Handle<JSObject*> aGivenProto) {
  return KaiOS_Binding::Wrap(cx, this, aGivenProto);
}

#ifdef MOZ_B2G_RIL
IccManager* KaiOS::GetMozIccManager(ErrorResult& aRv) {
  if (!mIccManager) {
    nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(mWindow);
    if (!global) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }
    mIccManager = new IccManager(mWindow);
  }

  return mIccManager;
}

Voicemail*
KaiOS::GetMozVoicemail(ErrorResult& aRv)
{
  if (!mVoicemail) {
    if (!mWindow) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }

    mVoicemail = Voicemail::Create(mWindow, aRv);
  }

  return mVoicemail;
}

MobileConnectionArray* KaiOS::GetMozMobileConnections(ErrorResult& aRv) {
  if (!mMobileConnections) {
    if (!mWindow) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }
    mMobileConnections = new MobileConnectionArray(mWindow);
  }

  return mMobileConnections;
}

Telephony* KaiOS::GetMozTelephony(ErrorResult& aRv) {
  if (!mTelephony) {
    nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(mWindow);
    if (!global) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }
    mTelephony = Telephony::Create(mWindow, aRv);
  }

  return mTelephony;
}

DataCallManager* KaiOS::GetDataCallManager(ErrorResult& aRv) {
  if (!mDataCallManager) {
    nsPIDOMWindowInner* win = GetWindow();
    if (!win) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }

    mDataCallManager = ConstructJSImplementation<DataCallManager>(
        "@mozilla.org/datacallmanager;1", win->AsGlobal(), aRv);
    if (aRv.Failed()) {
      return nullptr;
    }
  }
  return mDataCallManager;
}
#endif

}  // namespace dom
}  // namespace mozilla
