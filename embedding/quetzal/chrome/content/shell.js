console.log("shell.js starting...");

function handle_event(event) {
    console.log(`shell event ${event.type}`);
}

document.addEventListener("DOMContentLoaded", function bootstrap() {
    document.removeEventListener("DOMContentLoaded", bootstrap);

    // window.system_frame.addEventListener("mozbrowseropenwindow", handle_event);
    // window.system_frame.addEventListener("mozbrowsertitlechange", handle_event);
    // window.system_frame.addEventListener("mozbrowsermetachange", handle_event);
    // window.system_frame.addEventListener("mozbrowserloadstart", handle_event);
    // window.system_frame.addEventListener("mozbrowserloadend", handle_event);

    window.system_frame.src = "http://localhost:8081/system/index.html";
    window.system_frame.focus();
});

window.addEventListener("keydown", (event) => {
    console.log(`shell keydown ${event.key}`);
}, true);