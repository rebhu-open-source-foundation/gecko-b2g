// Bootstrap loading of the homescreen.

document.addEventListener(
  "DOMContentLoaded",
  () => {
    let wm = document.querySelector("gaia-wm");
    wm.open_frame("chrome://b2g/content/homescreen/index.html", {
      isHomescreen: true,
    });
  },
  { once: true }
);
