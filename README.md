# Blind Fetch

**Blind Fetch: Origin-Private Firmware Selection for OTA Updates**

Blind Fetch is an experimental OTA firmware-delivery protocol for constrained devices. It combines authenticated device access, firmware entitlement secrets (FES), oblivious key derivation, encrypted metadata, and fixed-budget retrieval to reduce the information an honest-but-curious origin can infer about the firmware selected by a client.

This repository contains the prototype, experimental datasets and analysis scripts, and a Tamarin symbolic model.

## Reproducing the paper's results

**An ESP32 or running server is not required to reproduce the analyses from the supplied datasets.** Run commands from the repository root: the scripts use relative paths into `Data/`.

### Requirements

- Python 3.11 or later recommended
- Packages in `Scripts/requirements.txt`
- Tamarin Prover for the separate formal-verification step

On Linux or WSL:

```bash
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install -r ./Scripts/requirements.txt
```

### Analysis scripts

| Command | Output |
| --- | --- |
| `python3 ./Scripts/auroc.py` | Direct firmware and slot-mediated inference, group-aware cross-validation, bootstrap confidence intervals, permutation tests, confusion matrices, and AUROC figure. |
| `python3 ./Scripts/IQR.py` | TLS/non-TLS TTFB and TTLB quartiles, IQRs, and box plots. |
| `python3 ./Scripts/memory.py` | Average peak and lowest available ESP32 heap for four policy/ordering configurations. |
| `python3 ./Scripts/timing.py` | Conventional `esp_https_ota` versus Blind Fetch mean execution times and standard deviations, with plot. |
| `python3 ./Scripts/regressor.py` | Firmware-size inference from encrypted metadata versus a plaintext static-manifest control, using linear and Random Forest regression. |

These scripts use recorded datasets in `Data/`. They do **not** collect new measurements. Plotting scripts may open interactive windows; close a window to proceed to the next plot. `auroc.py` performs 1,000 permutations per attack/configuration and 10,000 bootstrap samples for confidence intervals, so it may take considerable time.

With the current dataset, `regressor.py` reports Blind Fetch linear-regression MAE **398.04 bytes** and Random Forest MAE **415.44 bytes**.

`Scripts/heatmaps.py` is optional supplementary visualization of the slot-position matrices and is not required for the paper's primary results. It uses `seaborn`.

### Device energy measurements

The original FNB58 USB power-monitor recording is included in `Data/` in `.cfn` format and can be inspected using compatible FNB58 analysis software. The monitor sampled at **20 Hz**. The reported measurements cover **five complete Blind Fetch retrieval and firmware-writing executions**; an interrupted sixth execution is excluded. Measurements were taken at the USB board input and therefore include peripherals and voltage-regulation losses.

### Origin concurrency experiment

The paper also reports a concurrent-download benchmark covering 1 to 1,000 clients. The five analysis scripts above do not reproduce this benchmark. **Before publication, identify the corresponding dataset and the procedure used to derive the reported statistics.**

## Formal verification

The symbolic model is `Proofs/proof.spthy`. From the repository root:

```bash
tamarin-prover --prove ./Proofs/proof.spthy
```

Expected result: all six lemmas verified, with a standard subterm-convergence warning concerning OPRF correctness. Tested with **Tamarin Prover 1.13.0**. This establishes symbolic properties of the model, not timing indistinguishability or the absence of implementation side channels.

## Building and running the prototype

These instructions are for collecting **new** observations; they are not required for the offline analyses above.

### Server prerequisites

The server was developed for Ubuntu 24.04 and .NET 10. Install native dependencies:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config libsodium-dev openssl
```

Install the [.NET 10 SDK](https://learn.microsoft.com/en-us/dotnet/core/install/linux-ubuntu-install) if not using the project's containerized server environment.

### TLS certificate

Generate a certificate for the IP address or DNS name used by the ESP32. For an IP address, include an IP subject alternative name (SAN):

```bash
openssl req -x509 -newkey rsa:2048 -sha256 -days 365 -nodes \
  -keyout ./Server/esp.key -out ./Server/esp.crt \
  -subj '/CN=192.168.1.100' \
  -addext 'subjectAltName=IP:192.168.1.100'

openssl pkcs12 -export -out ./Server/esp.pfx \
  -inkey ./Server/esp.key -in ./Server/esp.crt

cp ./Server/esp.crt ./Firmware/certs/esp.crt
```

Replace `192.168.1.100` in both certificate fields with the reachable server IP, or use a `DNS:` SAN for a hostname. Supply the password selected during PFX export:

```bash
export BLIND_FETCH_TLS_PASSWORD='your-pfx-password'
```

Do not commit TLS private keys, PFX files, or passwords. The self-signed certificate above is intended for controlled testing.

### Native interop and server

From the repository root:

```bash
cmake -S ./Interop -B ./Interop/build
cmake --build ./Interop/build
```

The project build is expected to place the native output where the server can load it. Place `.bin` firmware images (filenames without spaces) in `./Server/Firmware/`. For each firmware family, generate a corresponding **32-byte (256-bit) FES** in `./Server/Keys/`:

```bash
head -c 32 /dev/urandom > ./Server/Keys/SKU.k
```

Replace `SKU` with the matching firmware identifier. The filename is an identifier; the contents of the `.k` file are secret. To run the conventional OTA comparison, place `firmware.bin` in `./Server/PlainFirmware/`.

For the experimental ordered-slot configuration, define `ORDERED` in `./Server/Program.cs` or compile with `-p:DefineConstants=ORDERED`. Otherwise use the randomized configuration.

Start the server from its directory:

```bash
cd ./Server
dotnet run
```

Ensure its listening address and TLS configuration match the device configuration.

### ESP32 firmware

The firmware uses [PlatformIO](https://platformio.org/install) and has been tested on **ESP32-WROOM-32D** and **ESP32-WROOM-32E** modules. Other ESP32 variants may require board-specific changes.

Create `./Firmware/.env` with **one comma-separated list of firmware names and one corresponding list of FES values**:

```dotenv
WIFI_SSID=your-wifi-ssid
WIFI_PASSWORD=your-wifi-password
OPAQUE_PASSWORD=unique-device-password
OPAQUE_SERVER_URL=https://192.168.1.100:5000
NUM_FIRMWARE_KEYS=4
FIRMWARE_NAMES=FW1,FW2,FW3,FW4
FIRMWARE_KEYS=HEX_KEY_FOR_FW1,HEX_KEY_FOR_FW2,HEX_KEY_FOR_FW3,HEX_KEY_FOR_FW4
```

Replace the placeholders with real values. **Each `HEX_KEY_FOR_FWn` must be exactly 64 hexadecimal characters**, encoding the 32 raw bytes from its corresponding `./Server/Keys/SKU.k` file. To obtain a key's hexadecimal representation on Linux:

```bash
xxd -p -c 256 ./Server/Keys/SKU.k
```

The lists are **positional**: `FIRMWARE_NAMES[i]` corresponds to `FIRMWARE_KEYS[i]`. `NUM_FIRMWARE_KEYS` must equal the number of entries in **both** lists. The firmware parses the comma-separated strings into its existing `firmwareNames[]` and `firmwareKeys[]` arrays. Keys remain hexadecimal strings when passed to the existing firmware-download functions. The four entries above are illustrative; the count and names are configured entirely through `.env`.

**Do not commit `.env` or the FES values.** In production, FES provisioning should use an appropriate device-secret storage mechanism rather than plaintext build configuration.

Configure the experimental build in `./Firmware/platformio.ini` using `build_flags`. **Keep `-DESPRESSIF_ESP32`.**

| Build flag | Purpose |
| --- | --- |
| `-DEarlyClose` | Use the early-close experimental policy instead of fixed-budget retrieval. |
| `-DReportUsage` | Record heap usage and send device metrics to the server, as used by the supplied `app_main()` implementation. |
| `-DMEMETRACE` | Enable UART heap tracing where supported by the firmware. |
| `-DHEADERONLY` | Request header generation without the full firmware download. |
| `-DPlainOTA` | Use the conventional OTA comparison path. |
| `-DSEND_KTVS` | Log sensitive protocol test vectors; **use only in an isolated test environment**. |
| `-DAPPLY_FIRMWARE` | Write the selected firmware to the OTA partition and restart; use a valid image for the target board. |

Upload and monitor:

```bash
cd ./Firmware
pio run --target upload --target monitor --environment esp32dev
```

The device must trust the server certificate, reach the configured server, and possess at least one FES matching a firmware available at the server.

## Scope and reproducibility notes

- Analysis scripts use the supplied datasets; executing the prototype produces new observations rather than reproducing the recorded runs exactly.
- Origin-private selection is evaluated under the paper's adversary model and experimental conditions; the statistical findings do not establish universal side-channel indistinguishability.
- The Tamarin model addresses symbolic protocol properties, not network timing, energy, or all implementation behaviour.
- Some experimental flags expose secrets or install firmware. Use disposable test credentials and an isolated network.

## Citation and license

Please cite the accompanying *Blind Fetch: Origin-Private Firmware Selection for OTA Updates* paper when using this artifact. Consult the repository's license file for reuse terms, if provided.
