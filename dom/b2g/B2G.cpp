/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/B2G.h"
#include "mozilla/dom/B2GBinding.h"

namespace mozilla {
namespace dom {

B2G::B2G(nsIGlobalObject* aGlobal) : mOwner(aGlobal) { MOZ_ASSERT(aGlobal); }
B2G::~B2G() {}

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(B2G)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(B2G)
NS_IMPL_CYCLE_COLLECTING_RELEASE(B2G)

NS_IMPL_CYCLE_COLLECTION_CLASS(B2G)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(B2G)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(B2G)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mOwner)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mAlarmManager)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mTetheringManager)
#ifdef MOZ_B2G_RIL
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mIccManager)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mVoicemail)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mMobileConnections)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mTelephony)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDataCallManager)
#endif
#ifdef HAS_KOOST_MODULES
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mExternalAPI)
#endif
#ifdef MOZ_B2G_BT
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mBluetooth)
#endif
#ifndef DISABLE_WIFI
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mWifiManager)
#endif
#ifdef MOZ_AUDIO_CHANNEL_MANAGER
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mAudioChannelManager)
#endif
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(B2G)

void B2G::Shutdown() {}

JSObject* B2G::WrapObject(JSContext* cx, JS::Handle<JSObject*> aGivenProto) {
  return B2G_Binding::Wrap(cx, this, aGivenProto);
}

AlarmManager* B2G::GetAlarmManager(ErrorResult& aRv) {
  if (!mAlarmManager) {
    if (!mOwner) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }

    mAlarmManager = new AlarmManager(GetParentObject());
  }

  return mAlarmManager;
}

TetheringManager* B2G::GetTetheringManager(ErrorResult& aRv) {
  if (!mTetheringManager) {
    if (!mOwner) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }

    mTetheringManager = ConstructJSImplementation<TetheringManager>(
        "@mozilla.org/tetheringmanager;1", GetParentObject(), aRv);
    if (aRv.Failed()) {
      return nullptr;
    }
  }

  return mTetheringManager;
}

#ifdef MOZ_B2G_RIL
IccManager* B2G::GetIccManager(ErrorResult& aRv) {
  if (!mIccManager) {
    if (!mOwner) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }

    mIccManager = new IccManager(GetParentObject());
  }

  return mIccManager;
}

CellBroadcast* B2G::GetCellBroadcast(ErrorResult& aRv) {
  if (!mCellBroadcast) {
    if (!mOwner) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }

    mCellBroadcast = CellBroadcast::Create(GetParentObject(), aRv);
  }

  return mCellBroadcast;
}

Voicemail*
B2G::GetVoicemail(ErrorResult& aRv)
{
  if (!mVoicemail) {
    if (!mOwner) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }

    mVoicemail = Voicemail::Create(GetParentObject(), aRv);
  }

  return mVoicemail;
}

MobileConnectionArray* B2G::GetMobileConnections(ErrorResult& aRv) {
  if (!mMobileConnections) {
    if (!mOwner) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }
    mMobileConnections = new MobileConnectionArray(GetParentObject());
  }

  return mMobileConnections;
}

Telephony* B2G::GetTelephony(ErrorResult& aRv) {
  if (!mTelephony) {
    if (!mOwner) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }
    mTelephony = Telephony::Create(GetParentObject(), aRv);
  }

  return mTelephony;
}

DataCallManager* B2G::GetDataCallManager(ErrorResult& aRv) {
  if (!mDataCallManager) {
    if (!mOwner) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }

    mDataCallManager = ConstructJSImplementation<DataCallManager>(
        "@mozilla.org/datacallmanager;1", GetParentObject(), aRv);
    if (aRv.Failed()) {
      return nullptr;
    }
  }
  return mDataCallManager;
}

SubsidyLockManager* B2G::GetSubsidyLockManager(ErrorResult& aRv) {
  if (!mSubsidyLocks) {
    if (!mOwner) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }
  }

  nsPIDOMWindowInner* innerWindow = mOwner->AsInnerWindow();
  if (!innerWindow) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  mSubsidyLocks = new SubsidyLockManager(innerWindow);

  return mSubsidyLocks;
}
#endif


#ifdef HAS_KOOST_MODULES
ExternalAPI* B2G::GetExternalapi(ErrorResult& aRv) {
  if (!mExternalAPI) {
    mExternalAPI = ExternalAPI::Create(mOwner);
  }

  return mExternalAPI;
}
#endif

#ifdef MOZ_B2G_BT
bluetooth::BluetoothManager* B2G::GetBluetooth(ErrorResult& aRv) {
  if (!mBluetooth) {
    if (!mOwner) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }
    nsPIDOMWindowInner* innerWindow = mOwner->AsInnerWindow();
    if (!innerWindow) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }

    mBluetooth = bluetooth::BluetoothManager::Create(innerWindow);
  }
  return mBluetooth;
}
#endif  // MOZ_B2G_BT

#ifndef DISABLE_WIFI
WifiManager* B2G::GetWifiManager(ErrorResult& aRv) {
  if (!mWifiManager) {
    if (!mOwner) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }
    mWifiManager = ConstructJSImplementation<WifiManager>(
        "@mozilla.org/wifimanager;1", GetParentObject(), aRv);
    if (aRv.Failed()) {
      return nullptr;
    }
  }
  return mWifiManager;
}
#endif

/* static */
bool B2G::HasWifiManagerSupport(JSContext* /* unused */, JSObject* aGlobal) {
#ifdef DISABLE_WIFI
  return false;
#endif
  // On XBL scope, the global object is NOT |window|. So we have
  // to use nsContentUtils::GetObjectPrincipal to get the principal
  // and test directly with permission manager.

  // TODO: permission check mechanism
  // nsIPrincipal* principal = nsContentUtils::ObjectPrincipal(aGlobal);
  //
  // nsCOMPtr<nsIPermissionManager> permMgr =
  //   services::GetPermissionManager();
  // NS_ENSURE_TRUE(permMgr, false);
  //
  // uint32_t permission = nsIPermissionManager::DENY_ACTION;
  // permMgr->TestPermissionFromPrincipal(principal,
  // NS_LITERAL_CSTRING("wifi-manage"), &permission); return
  // nsIPermissionManager::ALLOW_ACTION == permission;
  return true;
}

DownloadManager* B2G::GetDownloadManager(ErrorResult& aRv) {
  if (!mDownloadManager) {
    if (!mOwner) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }
    mDownloadManager = ConstructJSImplementation<DownloadManager>(
        "@mozilla.org/download/manager;1", GetParentObject(), aRv);
    if (aRv.Failed()) {
      return nullptr;
    }
  }
  return mDownloadManager;
}

#ifdef MOZ_AUDIO_CHANNEL_MANAGER
system::AudioChannelManager* B2G::GetAudioChannelManager(ErrorResult& aRv) {
  if (!mAudioChannelManager) {
    if (!mOwner) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }
    mAudioChannelManager = new system::AudioChannelManager();
    mAudioChannelManager->Init(mOwner);
  }
  return mAudioChannelManager;
}
#endif

}  // namespace dom
}  // namespace mozilla
