# Blind Fetch

**Blind Fetch: Origin-Private Firmware Selection for OTA Updates**

Blind Fetch is an experimental over-the-air (OTA) firmware-delivery protocol for constrained devices. It combines authenticated device access, high-entropy Firmware Entitlement Secrets (FES), oblivious pseudorandom function (OPRF) evaluation, encrypted firmware metadata, and fixed-budget retrieval to reduce what an honest-but-curious firmware origin can infer about the firmware selected by a client.

The prototype uses OPAQUE for authentication and session establishment, OPRF evaluation for entitlement-dependent key derivation, and authenticated encryption for firmware descriptors and payload blocks. Firmware candidates are placed into a padded bundle, and the client consumes the full bundle regardless of its selection. The origin knows the candidate firmware and their per-request ordering; the objective is to conceal **which candidate the authenticated client selects**, not the device's identity or the bundle contents.

This repository contains the ESP32 prototype, the .NET origin, experimental datasets and analysis scripts, and a Tamarin symbolic model. The paper evaluates reduced request-level selection leakage, **not** universal timing or side-channel indistinguishability.

## Reproducing the paper's results

**An ESP32 and a running origin are not required to reproduce the offline analyses from the supplied datasets.** Unless otherwise indicated, run commands from the repository root; scripts use relative paths under `Data/`.

### Requirements

- Python 3.11 or later recommended
- Python packages listed in `Scripts/requirements.txt`
- Tamarin Prover for the separate symbolic-verification step

On Linux or WSL:

```bash
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install -r ./Scripts/requirements.txt
```

### Analysis scripts

| Command | Purpose |
| --- | --- |
| `python3 ./Scripts/auroc.py` | Development-set slot-mediated and direct firmware-inference attacks; group-aware cross-validation, bootstrap confidence intervals, permutation tests, confusion matrices, and AUROC plots. |
| `python3 ./Scripts/heldout.py` | Independent held-out evaluation. Trains on `Data/MergedExecutions.csv`, tests on `Data/HeldOutExecutions.csv`, and reports inference performance with bootstrap uncertainty. |
| `python3 ./Scripts/IQR.py` | TLS and non-TLS TTFB/TTLB quartiles, interquartile ranges, and box plots. |
| `python3 ./Scripts/memory.py` | Average peak heap usage and minimum free heap for the four retrieval-policy and slot-ordering configurations. |
| `python3 ./Scripts/timing.py` | Conventional `esp_https_ota` versus Blind Fetch execution times, standard deviations, and plots. |
| `python3 ./Scripts/regressor.py` | Firmware-size inference from encrypted metadata versus a plaintext static-manifest control, using linear and Random Forest regression. |

The inference analyses distinguish **development-set cross-validation** from **independent held-out testing**. The held-out data is not used to train the models in `heldout.py`.

`auroc.py` runs 1,000 label permutations per attack/configuration and 10,000 bootstrap samples, so it may take considerable time. `heldout.py` also performs 10,000 bootstrap iterations. Plotting scripts may open interactive windows; close each window to proceed.

With the dataset used in the paper, `regressor.py` reports a Blind Fetch linear-regression mean absolute error (MAE) of **398.04 bytes** and Random Forest MAE of **415.44 bytes**. The regression test examines encrypted descriptor features; it does not establish that the origin is unaware of candidate firmware sizes, which the threat model assumes the origin already knows.

`Scripts/heatmaps.py` is an optional supplementary visualization of slot-position matrices, not required for the primary reported results. It uses `seaborn`.

### Device energy measurements

FNB58 USB power-monitor recordings are supplied in `Data/` in `.cfn` format for inspection with compatible FNB58 software. The monitor sampled at **20 Hz**. Measurements were taken at the USB board input and therefore include peripherals and voltage-regulation losses.

The manuscript reports **four complete executions per configuration** in its energy comparison. Raw recordings may include additional or interrupted executions; they should not automatically be included in those summary statistics. The Python scripts below do not parse `.cfn` recordings or independently regenerate the energy table. **Before freezing the archive, identify the exact recordings and intervals used for each reported value**, so that the count and calculations can be independently verified.

### Origin concurrency benchmark

The paper reports concurrent-download batches ranging from **1 to 1,000 clients**, with **1,941 successful downloads**. This is a separate load-generator benchmark, not an ESP32 device experiment. The analysis commands above do **not** reproduce it. Repeating the benchmark requires the original load-generator setup, server configuration, and execution logs. The origin writes request-level performance metrics to `Logs/ServerMetrics.csv` in its working directory; these runtime logs are not, by themselves, a complete reproduction procedure for the published concurrency experiment. If the benchmark inputs or logs are not included in the archived release, treat this result as **not independently reproducible from the repository alone**.

## Formal verification

The symbolic model is `Proofs/proof.spthy`. From the repository root:

```bash
tamarin-prover --prove ./Proofs/proof.spthy
```

The paper reports **six security lemmas and six reachability lemmas**, automatically discharged using **Tamarin Prover 1.13.0**. Tamarin may emit a subterm-convergence warning concerning OPRF correctness.

The model establishes properties of its symbolic protocol and encoded assumptions. It does **not** verify network timing, fixed-length implementation behaviour, physical-device security, or the absence of implementation side channels.

## Building and running the prototype

These instructions are for collecting **new** observations; they are not required for offline analysis. Network conditions, device configuration, and machine load may cause new timing measurements to differ from the published dataset.

### Server prerequisites

The origin was developed for Ubuntu 24.04 with .NET 10. Install native build dependencies:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config libsodium-dev openssl
```

Install the [.NET 10 SDK](https://learn.microsoft.com/en-us/dotnet/core/install/linux-ubuntu-install) if you are not using the project's containerized environment.

### TLS certificate for local testing

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

Replace `192.168.1.100` with the server's reachable IP in **both** certificate fields, or use a `DNS:` SAN for a hostname. Supply the password selected during PFX export:

```bash
export BLIND_FETCH_TLS_PASSWORD='your-pfx-password'
```

The server reads `BLIND_FETCH_TLS_PASSWORD` and loads `./esp.pfx` from its working directory. Start it from `./Server` so the relative certificate path resolves correctly. Do not commit TLS private keys, PFX files, or passwords. The self-signed certificate is intended for controlled testing only.

### Native interop and origin

From the repository root:

```bash
cmake -S ./Interop -B ./Interop/build
cmake --build ./Interop/build
```

Ensure the resulting native library is available to the .NET server at runtime.

Place `.bin` firmware images (filenames without spaces) in `./Server/Firmware/`. For each firmware family, create a corresponding **32-byte (256-bit) FES** in `./Server/Keys/`:

```bash
head -c 32 /dev/urandom > ./Server/Keys/SKU.k
```

Replace `SKU` with the matching firmware identifier. The filename is an identifier; the `.k` contents are secret. To run the conventional OTA comparison, place `firmware.bin` in `./Server/PlainFirmware/`.

For the experimental ordered-slot configuration, define `ORDERED` in `./Server/Program.cs` or compile with `ORDERED` defined. Otherwise use randomized placement. When using `-p:DefineConstants=...`, check that the override preserves any other required compilation constants.

Start the origin from its directory:

```bash
cd ./Server
dotnet run
```

Ensure its listening address, HTTPS certificate, native library path, and firmware/key directories match the configuration used by the ESP32.

### ESP32 firmware

The firmware uses [PlatformIO](https://platformio.org/install) and targets the **ESP32-WROOM-32D**; it has also been run on **ESP32-WROOM-32E** modules. Other ESP32 variants may require board-specific changes.

Create `./Firmware/.env`:

```dotenv
WIFI_SSID=your-wifi-ssid
WIFI_PASSWORD=your-wifi-password
OPAQUE_PASSWORD=unique-device-password
OPAQUE_SERVER_URL=https://192.168.1.100:5000
NUM_FIRMWARE_KEYS=4
FIRMWARE_NAMES=FW1,FW2,FW3,FW4
FIRMWARE_KEYS=HEX_KEY_FOR_FW1,HEX_KEY_FOR_FW2,HEX_KEY_FOR_FW3,HEX_KEY_FOR_FW4
```

Replace the placeholders with real values. Each `HEX_KEY_FOR_FWn` must contain **exactly 64 hexadecimal characters**, encoding the 32 bytes in its corresponding server `.k` file. To convert a key to hexadecimal on Linux:

```bash
xxd -p -c 256 ./Server/Keys/SKU.k
```

The lists are **positional**: `FIRMWARE_NAMES[i]` corresponds to `FIRMWARE_KEYS[i]`, and `NUM_FIRMWARE_KEYS` must equal the number of entries in each list. The firmware parses these comma-separated values into its `firmwareNames[]` and `firmwareKeys[]` arrays. The keys remain hexadecimal strings when passed to the existing download functions.

**Experimental provisioning versus deployment:** Multiple names and FES values are supplied to the experimental firmware so that the test harness can exercise different firmware identities across repeated measurements. This must not be confused with the intended entitlement model: a deployed device should be provisioned only with the FES values for firmware families it is authorized to retrieve. The OPAQUE password authenticates the device; it is not a firmware entitlement secret.

Do not commit `.env` or FES values. Production deployments should use appropriate secret provisioning and device storage rather than plaintext build configuration.

### Experimental build flags

**The checked-in `./Firmware/platformio.ini` enables `-DPlainOTA` by default. Remove or comment out `-DPlainOTA` before building Blind Fetch.** Leave it enabled only when measuring the conventional OTA baseline. Configure the remaining flags using `build_flags` and **keep `-DESPRESSIF_ESP32`**.

| Flag | Purpose |
| --- | --- |
| `-DEarlyClose` | Use early-close retrieval instead of the fixed-budget policy. |
| `-DReportUsage` | Record heap usage and send device metrics to the origin. |
| `-DMEMETRACE` | Enable UART heap tracing where supported. |
| `-DHEADERONLY` | Request headers without downloading the full firmware bundle. |
| `-DPlainOTA` | Use the conventional OTA comparison path. |
| `-DSEND_KTVS` | Log sensitive protocol test vectors; **isolated test environments only**. |
| `-DAPPLY_FIRMWARE` | Write selected firmware to the OTA partition and restart; requires a valid image for the board. |

Upload and monitor:

```bash
cd ./Firmware
pio run --target upload --target monitor --environment esp32dev
```

The device must trust the origin's certificate, reach its configured address, and have at least one FES corresponding to a firmware family available at the origin.

## Reproducibility and security scope

- **Offline reproduction:** The Python scripts recompute analyses from recorded datasets; they do not collect new measurements.
- **Independent validation:** `heldout.py` trains on the development dataset and evaluates on a separate held-out dataset.
- **Symbolic verification:** Tamarin checks properties of the encoded model, not physical-device or timing behaviour.
- **Experimental replication:** Building and running the prototype collects new data and may produce different timings due to environmental conditions.
- **Privacy scope:** The origin knows the authenticated client, candidate firmware families, and per-request slot permutation. The measured reduction in selection leakage does not imply universal side-channel indistinguishability.
- **Entitlement scope:** FES secrecy and trusted initial provisioning are assumptions. Family-wide FES compromise, physical extraction, rotation, and revocation are not solved by this prototype.
- **Operational safety:** Some build flags expose secrets or install firmware. Use disposable test credentials, appropriate test hardware, and an isolated network.

## Archival reproducibility

For a permanent research snapshot, cite an immutable Git release or archive DOI rather than a moving branch. The archive should include the precise datasets used for all reported analyses, the scripts and requirements, the symbolic model, and the corresponding source revision. Do not archive real device credentials, FES values, TLS private keys, or sensitive protocol traces.

## Citation and license

If you use this artifact, please cite the accompanying paper, *Blind Fetch: Origin-Private Firmware Selection for OTA Updates*.

The repository is distributed under the **BSD 3-Clause License**. See [`LICENSE`](LICENSE) for the full terms.
