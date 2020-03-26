# Download API

The **Download API** help app to manage download tasks. A download task might be initialized by a click on a link with download attribute, or a click on the link with file extensions that cannot be viewed directly.

## Get Download List
```javascript
navigator.b2g.downloadManager.getDownloads().then(
  list => {
    console.log("get download list: " + list);
  },
  err => {
    console.log("get download list failed. err: " + err);
  }
);
```

## Remove a Download Object
```javascript
navigator.b2g.downloadManager.getDownloads().then(
  list => {
    if (list.length) {
    navigator.b2g.downloadManager.remove(list[0]).then(
      download => {
        console.log("download removed. id: " + download.id);
      },
      err => {
        console.log("remove download failed. err: " + err);
      }
    );
    } else {
      console.log("download list is empty, nothing to remove.");
    }
  },
  err => {
    console.log("get download list failed. err: " + err);
  }
)
```

## Clear All Finished Downloads
```javascript
navigator.b2g.downloadManager.clearAllDone();
```

## Adopt Download
```javascript
navigator.b2g.downloadManager.adoptDownload(
  {
    "totalBytes": 1024,
    "url": "", // This can be empty, or the app url that adopts this download.
    "storageName": "data/usbmsc_mnt",
    "storagePath": "downloads/download.txt", // This file should actually exist.
    "contentType": "text/plain",
    "startTime": new Date(Date.now())
  }
);
```

## Listen to Start Event of New Downloads
```javascript
navigator.b2g.downloadManager.addEventListener(
  'downloadstart',
  evt => console.log("downloadstart: " + evt.download)
);
```

## Listen to State Change of a Download
```javascript
navigator.b2g.downloadManager.addEventListener(
  'downloadstart',
  evt => {
    console.log("downloadstart: " + evt.download.id + " " + evt.download.url);
    evt.download.addEventListener(
      'statechange',
      (st) => console.log("download state change. " + st.download.id + " " + st.download.state)
    );
  }
);
```

## Notes
1. The App permission of Download API is not yet handled.
2. In Gecko48, user can click on the `Download` option from context menu on an image, to download that image. This is not supported yet.
3. In Gecko48, Gaia also maintains a list of all completed downloads in its own db, and updates every time a download is completed. To avoid duplication, System App calls clearAllDone every time b2g starts. This behavior is not settled yet.
