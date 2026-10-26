"use strict";

const isoInput = document.getElementById("iso-file");
const isoStatus = document.getElementById("iso-status");
const runButton = document.getElementById("run-demo");
const consoleOutput = document.getElementById("console");
const demoStatus = document.getElementById("demo-status");
const progressBar = document.getElementById("progress-bar");

const bootMessages = [
  "firmware: handing off to Limine (simulated)",
  "limine: x86_64 protocol selected",
  "limine: framebuffer request completed",
  "akernel: initializing framebuffer console",
  "akernel: loading fetch system information",
  "akernel: startup display ready",
  "akernel: halted (demo complete)"
];

const fetchArt = [
  "                   ###-",
  "          .---##+++#+#.",
  "          .-#####--+-+-+",
  "          .-#-++#++# +-#####+-",
  "   .#######-###+## ####-+--.--",
  "   -#####-#-+--#-+#+  +#####+-",
  "   .####+##-   +#-.#.  ----",
  "    +##-.        +#+-+- + +",
  "            +#+   .--.+---++",
  "   .###    .#.#     -+--####",
  "###-#.#++--###+---   .-#.#.#.",
  "#-#-###- +-#+#.- - +#- +####.         ...",
  "###-.--++--##+---- # #   +##+##.     ###.",
  "             .+.+++.#+      ######+ -#-#.",
  "              -+-+.+        +##-###.####",
  "                .++-         +####-.#-#.",
  "                               -+- .###",
  "                                 ..."
];

const fetchInfo = [
  "akernel",
  "-----------------------------",
  "OS: akernel",
  "Version: 1.2.1 (Slim Jims)",
  "Architecture: x86_64",
  "Build: freestanding ELF",
  "Status: kernel scaffold",
  "Packages: metadata and stubs",
  "Boot: Limine"
];

let timerId = null;

isoInput.addEventListener("change", async () => {
  const file = isoInput.files && isoInput.files[0];
  if (!file) {
    isoStatus.textContent = "No image selected.";
    isoStatus.dataset.state = "";
    return;
  }

  try {
    const descriptorType = new Uint8Array(
      await file.slice(16 * 2048, 16 * 2048 + 1).arrayBuffer()
    );
    const descriptor = new Uint8Array(
      await file.slice(16 * 2048 + 1, 16 * 2048 + 6).arrayBuffer()
    );
    const version = new Uint8Array(
      await file.slice(16 * 2048 + 6, 16 * 2048 + 7).arrayBuffer()
    );
    const signature = String.fromCharCode(...descriptor);

    if (file.size < 16 * 2048 + 7 ||
        descriptorType[0] !== 1 ||
        signature !== "CD001" ||
        version[0] !== 1) {
      isoStatus.textContent =
        `${file.name}: no ISO 9660 primary-volume descriptor was found.`;
      isoStatus.dataset.state = "error";
      return;
    }

    isoStatus.textContent =
      `${file.name}: ISO 9660 primary-volume descriptor found (${formatBytes(file.size)}). This does not test bootability.`;
    isoStatus.dataset.state = "success";
  } catch (error) {
    isoStatus.textContent = `Could not read the selected image: ${error.message}`;
    isoStatus.dataset.state = "error";
  }
});

runButton.addEventListener("click", () => {
  if (timerId !== null) {
    window.clearTimeout(timerId);
    timerId = null;
  }

  runButton.disabled = true;
  consoleOutput.textContent = "akernel browser preview\nThis is not a real kernel boot.\n";
  demoStatus.textContent = "Running simulated startup…";
  progressBar.style.width = "0%";
  playMessage(0);
});

function playMessage(index) {
  if (index >= bootMessages.length) {
    demoStatus.textContent = "Demo complete. No kernel was executed.";
    runButton.disabled = false;
    timerId = null;
    return;
  }

  consoleOutput.textContent += `${bootMessages[index]}\n`;
  if (index === 5) {
    consoleOutput.textContent += `\n${renderFetchOutput()}\n`;
  }
  consoleOutput.scrollTop = consoleOutput.scrollHeight;
  progressBar.style.width = `${Math.round(((index + 1) / bootMessages.length) * 100)}%`;
  timerId = window.setTimeout(() => playMessage(index + 1), 450);
}

function formatBytes(bytes) {
  return `${(bytes / (1024 ** 2)).toFixed(1)} MiB`;
}

function renderFetchOutput() {
  return fetchArt.map((art, row) => {
    const info = fetchInfo[row] || "";
    return `${art.padEnd(44, " ")}${info ? `  ${info}` : ""}`;
  }).join("\n");
}
