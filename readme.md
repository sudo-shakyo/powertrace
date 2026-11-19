# PowerTrace

A lightweight, multi-threaded Linux daemon written in C designed to monitor AC power states, correlate GPU display disconnect events, and actively protect external displays from electrical transients.

PowerTrace solves two critical systems problems: accurately distinguishing between a pure hardware fault (e.g., an unplugged HDMI-VGA adapter) and an upstream power failure (e.g., a brownout or charger disconnection), while actively suspending the HDMI link during power drops to shield the display controller from voltage spikes and ground-loop noise.

---

## System Architecture

PowerTrace utilizes a concurrent, thread-safe architecture to decouple real-time hardware telemetry sampling, event correlation, and active display protection:

1. **Power Sampler Thread (`sampler.c`)**:
   - Dynamically discovers the active power path by scanning `/sys/class/power_supply/` at runtime (supporting adapters like `AC`, `ADP`, or `ACAD`).
   - Runs a 10Hz sampling loop, recording the online status and timestamps into a thread-safe, mutex-locked circular ring buffer.
   - Monitors `ac_online` state transitions (`1` → `0`) to immediately trigger the HDMI protection worker on power drops or voltage spikes.

2. **Active HDMI Protector (`sampler.c`)**:
   - Spawns a non-blocking, detached POSIX worker thread (`pthread`) so the 10Hz telemetry sampler never stalls.
   - Locates active external HDMI interfaces via `glob("/sys/class/drm/card*-HDMI-*/status")` and writes `off` to immediately halt low-voltage GPU signaling.
   - Enforces a 10-second settling buffer to let mains electrical transients and power supply capacitors stabilize.
   - Writes `detect` back to the kernel DRM interface to re-initiate the HDMI handshake and automatically restore the display.

3. **Event Correlator Thread (`correlator.c`)**:
   - Listens for kernel-level DRM (Direct Rendering Manager) display disconnect events (via `nvidia-drmdrmfb` logs).
   - Upon receiving a disconnect signal, it locks the buffer and scans a historical 5-second time window to verify whether an AC power drop occurred simultaneously.

---

## False-Positive Suppression & Idle Verification

To ensure system stability during normal operation, PowerTrace undergoes idle validation to verify that background battery transitions or minor system ticks do not trigger false alerts.

As captured during idle test runs, when the daemon runs without fault injection or external disconnects, it maintains zero false positives:

```text
[shakyo@chronos powertrace_project]$ sudo ./powertrace
Starting PowerTrace Daemon...
[Sampler] Dynamically bound to power sensor: ACAD
^C
--- PowerTrace Final Report ---
Total Dropouts: 0
Power-related: 0
Likely Hardware: 0
Verdict: Inconclusive. Not enough data.
PowerTrace has stopped safely.
```

*(Idle Verification Terminal Output)*  
![PowerTrace Idle Test Results](idle_test.png)

---

## Empirical Test Results (`powertrace_trial_results.csv`)

The repository includes a sample dataset (`powertrace_trial_results.csv`) logging multiple validation trials under different physical fault conditions:

- **Trials 1 – 5 (Voltage Fluctuation / Power Faults - Class 0):**
  Simulated or captured environmental power drops where the AC power state changed to `0` within the 5-second correlation window of a display interruption. The daemon successfully classified these as upstream power-related events.
- **Trials 6 – 7 (HDMI-VGA Adapter Removal - Class 1):**
  Physical disconnection trials where the HDMI-VGA adapter was unplugged while AC power remained entirely stable (`1`). The daemon scanned the ring buffer, found no power anomalies, and correctly classified them as pure hardware faults.

---

## Hardware Limitations & Engineering Findings

During live hardware testing, a significant physical limitation was uncovered: certain external display controllers maintain enough residual capacitance to keep the logical **HDMI Hot Plug Detect (HPD)** pin active during minor power brownouts.

This causes the hardware to mask the display failure from the OS, leaving the kernel completely silent. PowerTrace documents this hardware-software boundary limitation and handles it cleanly via strict historical correlation and verified log-injection testing methodologies.

---

## Project Structure

```text
powertrace/
├── powertrace.h                  # Shared data structures, ring buffer bounds, and extern declarations
├── sampler.c                     # 10Hz telemetry sampler, dynamic path discovery, and 10s DRM HDMI protector
├── correlator.c                  # Historical ring buffer scanner and dual-verification logic
├── main.c                        # Thread initialization, mutex management, and signal handlers
├── powertrace_trial_results.csv  # Empirical dataset containing voltage drop and adapter removal trials
├── idle_test.png                 # Screenshot showing zero false positives during idle verification
├── Makefile                      # Build automation recipe
└── README.md                     # Project documentation
```

---

## How to Download and Use

### 1. Prerequisites
Ensure you have a Linux environment with a C compiler (`gcc`) and `make` installed:
```bash
sudo pacman -S base-devel        # On Arch / CachyOS
# or sudo apt install build-essential  # On Debian / Ubuntu
```

### 2. Clone the Repository
Clone the project from GitHub to your local machine using git:
```bash
git clone [https://github.com/sudo-shakyo/powertrace.git](https://github.com/sudo-shakyo/powertrace.git)
cd powertrace
```

### 3. Build the Daemon
Compile the source files using the provided Makefile:
```bash
make
```
*(Or compile manually with `gcc -o powertrace *.c -lpthread -lm`)*

### 4. Run the Daemon
Because PowerTrace reads kernel power telemetry and writes directly to `/sys/class/drm/` to suspend/restore the HDMI link, execute the binary with root privileges:
```bash
sudo ./powertrace
```

### 5. Stopping the Daemon & Viewing the Report
Press `Ctrl + C` at any time to gracefully shut down the daemon. Upon exit, it will securely flush its state and print the final structured summary report detailing recorded dropouts, classified events, and verification verdicts.