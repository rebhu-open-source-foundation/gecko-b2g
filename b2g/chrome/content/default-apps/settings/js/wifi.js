class WifiManager {
  constructor() {
    this.manager = navigator.b2g.wifiManager;
  }

  get enabled() {
    return this.manager.enabled;
  }

  enable() {
    if (this.manager.enabled) {
      return Promise.resolve();
    }

    return new Promise((resolve, reject) => {
      let req = this.manager.setWifiEnabled(true);
      req.onsuccess = resolve;
      req.onerror = event => {
        reject(event.target.error);
      };
    });
  }

  disable() {
    if (!this.manager.enabled) {
      return Promise.resolve();
    }

    return new Promise((resolve, reject) => {
      let req = this.manager.setWifiEnabled(false);
      req.onsuccess = resolve;
      req.onerror = event => {
        reject(event.target.error);
      };
    });
  }

  getNetworks() {
    return new Promise((resolve, reject) => {
      let req = this.manager.getNetworks();
      req.onsuccess = () => {
        resolve(req.result);
      };
      req.onerror = () => {
        console.log(`WIFI Failed to find wifi networks: ${req.error}`);
        reject(req.error);
      };
    });
  }

  associate(network) {
    return this.manager.associate(network);
  }
}

function onNetworks(networks, manager) {
  console.log(`WIFI Found ${networks.length} wifi networks:`);
  let list = document.getElementById("network-list");
  let html = "";
  networks.forEach((network, index) => {
    if (network.ssid.trim().length == 0) {
      return;
    }
    console.log(`WIFI ${network.ssid} ${network.getSecurity()}`);
    html += `<li>${network.ssid} `;
    let security = network.getSecurity();
    if (security.length == 0) {
      html += `<button id="wifi-network-${index}-connect">Connect</button>`;
    } else {
      html += `(${security})`;
    }
    html += `</li>`;
  });
  list.innerHTML = html;
  networks.forEach((network, index) => {
    let security = network.getSecurity();
    // Only try to associate to open networks.
    if (network.ssid.trim().length == 0 || security.length != 0) {
      return;
    }
    document.getElementById(`wifi-network-${index}-connect`).onclick = () => {
      console.log(`WIFI Trying to associate with ${network.ssid}`);
      network.keyManagement = 4; // "NONE"
      console.log(
        `WIFI object is ${network} ${network.ssid} ${network.keyManagement}`
      );

      manager.associate(network).then(
        result => {
          console.log(`WIFI associate with ${network.ssid}: ${result}`);
        },
        error => {
          console.error(`WIFI Error: ${error}`);
        }
      );
    };
  });
}

function startWifi() {
  let manager = new WifiManager();
  console.log(`WIFI starting, enabled? ${manager.enabled}`);

  manager.enable().catch(error => {
    console.error(`WIFI Error: ${error}`);
  });

  return manager;
}

document.addEventListener(
  "DOMContentLoaded",
  () => {
    let manager = startWifi();
    document.getElementById("scan-wifi").onclick = () => {
      manager.getNetworks().then(
        networks => {
          onNetworks(networks, manager);
        },
        error => {
          console.log(`WIFI error: ${error}`);
        }
      );
    };
  },
  { once: true }
);
