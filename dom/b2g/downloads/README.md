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
    "url": "http://email.localhost", // This cannot be empty, or the download record cannot persist after b2g restart.
    "storageName": "sdcard",
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
The functions above are all available to apps with `downloads` permission. As for apps or webpages without `downloads` permission, getDownloads() and `downloadstart` events are still available to access the download objects for which it is the referrer.
