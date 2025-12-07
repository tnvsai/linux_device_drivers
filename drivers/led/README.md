# 1. Device Tree overlay — what, why, how

**File:** `sai-led-overlay.dts`

```dts
/dts-v1/;
/plugin/;

/ {
    compatible = "brcm,bcm2835";

    fragment@0 {
        target-path = "/";
        __overlay__ {
            sai_led: sai-led {
                compatible = "sai,my-led";
                gpios = <&gpio 17 0>;
                default-state = "on";
                status = "okay";
            };
        };
    };
};
```

## Explanation (line-by-line / block-by-block)

### `/dts-v1/;`
- **What:** DTS header that identifies the file version.
- **Why:** Required by the Device Tree Compiler (dtc).
- **How:** Always include as first line.

### `/plugin/;`
- **What:** Marks this DTS as an overlay (a plugin for the base tree).
- **Why:** Tells dtc and the bootloader that this file is an overlay to be applied to the base device tree.
- **How:** Use for overlays that patch the running DT.

### `compatible = "brcm,bcm2835";`
- **What:** Declares the SoC family this overlay applies to (Raspberry Pi / Broadcom).
- **Why:** Prevents overlay from being applied to wrong platforms.
- **How:** Keep this matching your Pi platform—standard for Raspberry Pi overlays.

### `fragment@0 { target-path = "/"; __overlay__ { ... } };`
- **What:** A fragment describes the place in the base tree to patch; `target-path = "/"` means apply at the root.
- **Why:** We want to add a new node at the root (not inside another node).
- **How:** Use `target = <&something>` or `target-path` depending on where to attach.

### `sai_led: sai-led { ... };`
- **What:** Defines a node named `sai-led` and labels it `sai_led` (label for reference).
- **Why:** The node name (`sai-led`) is arbitrary but convenient; label can be used within DTS if needed.
- **How:** Node name with optional `@<addr>` is allowed. Using `sai-led` (no `@`) avoids warnings.

### `compatible = "sai,my-led";`
- **What:** The key line for binding: it tells Linux that this node should be handled by drivers that claim `.compatible = "sai,my-led"`.
- **Why:** Kernel uses the compatible string to match DT nodes to drivers (via `of_match_table` and `MODULE_DEVICE_TABLE`).
- **How:** Choose a unique prefix (your namespace) like `sai,<device>`.

### `gpios = <&gpio 17 0>;`
- **What:** Tells kernel this device uses GPIO controller `&gpio`, GPIO number 17 (BCM17), and flags 0 (active-high).
- **Why:** The driver will request this gpio from DT using `devm_gpiod_get()`.
- **How:** Use controller phandle + number + flags; numeric flags (0/1) work without including dt-bindings headers.

### `default-state = "on";`
- **What:** A custom property the driver can read (optional) — here instructs driver to turn LED on on probe.
- **Why:** Shows how DT can pass driver-specific configuration values.
- **How:** Driver reads via `of_property_read_string()`.

### `status = "okay";`
- **What:** Marks the node enabled.
- **Why:** Common DT pattern; not strictly necessary for overlays but idiomatic.
- **How:** Use `status = "disabled";` to disable a node.

# 2. Compile overlay & install — commands & meaning

**Compile overlay to binary `.dtbo`**:

```bash
dtc -@ -I dts -O dtb -o sai-led.dtbo sai-led-overlay.dts
```

- `dtc` = Device Tree Compiler
- `-@`: includes symbols/strings in the DTB (helps kernel/device code)
- `-I dts`: input format = dts
- `-O dtb`: output format = dtb (binary)
- `-o sai-led.dtbo`: write output to that file

**Copy to overlays folder**:

```bash
sudo cp sai-led.dtbo /boot/overlays/
```
*Copies overlay to the Pi overlay folder so the bootloader knows about it.*

**Enable in config**:

```bash
echo "dtoverlay=sai-led" | sudo tee -a /boot/firmware/config.txt
```
*Add a line to the Pi's active `config.txt` so overlay is applied at boot. (On some systems `/boot/config.txt` is used; on newer Debian/Raspberry Pi OS it's `/boot/firmware/config.txt`.)*

**Reboot**:

```bash
sudo reboot
```
*Reboot so the overlay is applied and kernel constructs the device tree nodes.*

# 3. Verify DT node presence

After reboot:

```bash
dtc -I fs /sys/firmware/devicetree/base | grep -A5 sai-led
```

`dtc -I fs` pretty-prints the live device tree from sysfs. This shows that your overlay is applied and displays the node.

**Example expected output (simplified):**

```dts
sai-led {
    compatible = "sai,my-led";
    default-state = "on";
    status = "okay";
    gpios = <0x07 0x11 0x00>;
};
```
*Interpretation: node is present, gpios phandle & gpio index & flags are present.*

# 4. Kernel driver — full file and then deep explanation

**File:** `sai_led_driver.c`

```c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>

struct sai_led_priv {
    struct gpio_desc *led_gpio;
};

static int sai_led_probe(struct platform_device *pdev)
{
    struct sai_led_priv *priv;
    const char *state;

    pr_info("sai_led: probe called\n");

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->led_gpio = devm_gpiod_get(&pdev->dev, NULL, GPIOD_OUT_LOW);
    if (IS_ERR(priv->led_gpio)) {
        dev_err(&pdev->dev, "failed to acquire gpio\n");
        return PTR_ERR(priv->led_gpio);
    }

    if (!of_property_read_string(pdev->dev.of_node, "default-state", &state)) {
        if (!strcmp(state, "on"))
            gpiod_set_value(priv->led_gpio, 1);
        else
            gpiod_set_value(priv->led_gpio, 0);

        pr_info("sai_led: default-state=%s\n", state);
    }

    platform_set_drvdata(pdev, priv);
    pr_info("sai_led: probe success\n");
    return 0;
}

static void sai_led_remove(struct platform_device *pdev)
{
    pr_info("sai_led: remove called\n");
}

static const struct of_device_id sai_led_of_table[] = {
    { .compatible = "sai,my-led" },
    { }
};
MODULE_DEVICE_TABLE(of, sai_led_of_table);

static struct platform_driver sai_led_driver = {
    .probe = sai_led_probe,
    .remove = sai_led_remove,
    .driver = {
        .name = "sai_led",
        .of_match_table = sai_led_of_table,
    },
};

module_platform_driver(sai_led_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sai");
MODULE_DESCRIPTION("Simple LED driver using Device Tree + Platform driver");
```

## Driver explanation — what / why / how (step-by-step)

### Includes
```c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
```
- **What:** Pull in kernel APIs used by this driver.
- **Why:** `module.h` for module macros/logging; `platform_device.h` for probe signature and helper functions; `of.h` for Device Tree helpers; `gpio/consumer.h` for modern GPIO consumer API.
- **How:** These headers provide the functions/types used later.

### Private data struct
```c
struct sai_led_priv {
    struct gpio_desc *led_gpio;
};
```
- **What:** Driver-specific state stored per-device.
- **Why:** If driver needs multiple values (GPIO descriptors, counters, etc.), store them here and attach to the platform device via `platform_set_drvdata()`. Keeps driver re-entrant/multidevice-friendly.
- **How:** `gpio_desc` is the handle for the GPIO returned by `devm_gpiod_get()`.

### Probe()
```c
static int sai_led_probe(struct platform_device *pdev)
```
- **What:** Called by kernel when a platform device matches this driver's `of_match_table`.
- **Why:** Initialization happens here — request resources, configure hardware, start work timers, etc.
- **How:** Kernel calls this function after matching; it returns 0 on success or negative error code on failure.

#### Inside probe:

**Log start:**
```c
pr_info("sai_led: probe called\n");
```
- **What:** Kernel log stating that probe started.
- **Why:** Helpful debug log — you can see whether probe runs in dmesg.
- **How:** `pr_info()` is preferred over `printk()`.

**Allocate memory:**
```c
priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
if (!priv)
    return -ENOMEM;
```
- **What:** Allocate driver private data with device-managed allocator.
- **Why:** `devm_` allocations are automatically freed when device is removed; reduces cleanup code.
- **How:** `kzalloc` zero-initializes memory; `devm_kzalloc` ties its lifecycle to `pdev->dev`.

**Get GPIO:**
```c
priv->led_gpio = devm_gpiod_get(&pdev->dev, NULL, GPIOD_OUT_LOW);
if (IS_ERR(priv->led_gpio)) {
    dev_err(&pdev->dev, "failed to acquire gpio\n");
    return PTR_ERR(priv->led_gpio);
}
```
- **What:** Request the GPIO described in DT. Passing `NULL` mean "use the first (default) GPIO property" (here `gpios`). `GPIOD_OUT_LOW` configures the GPIO as output and sets it low initially.
- **Why:** Keeps driver DT-driven (no hard-coded GPIO numbers inside driver). Using `devm_gpiod_get()` makes the GPIO automatically released on device remove.
- **How:** If GPIO is missing or request fails, `devm_gpiod_get()` returns an `ERR_PTR` (check with `IS_ERR()` and return `PTR_ERR()`).

**Read DT property:**
```c
if (!of_property_read_string(pdev->dev.of_node, "default-state", &state)) {
    if (!strcmp(state, "on"))
        gpiod_set_value(priv->led_gpio, 1);
    else
        gpiod_set_value(priv->led_gpio, 0);

    pr_info("sai_led: default-state=%s\n", state);
}
```
- **What:** Read the optional DT property `default-state`. If present and equals "on", set LED high; otherwise set low.
- **Why:** Shows how DT configuration passes configuration values to driver — flexible and avoids recompiling for different hardware behavior.
- **How:** `of_property_read_string()` returns 0 on success. `gpiod_set_value()` sets the GPIO value.

**Set driver data:**
```c
platform_set_drvdata(pdev, priv);
pr_info("sai_led: probe success\n");
return 0;
```
- **What:** Save pointer to `priv` in platform device so `remove()` or other callbacks can retrieve it. Log success; return success.
- **Why:** `platform_get_drvdata()` can later fetch this pointer if `remove()` or other routines need it.
- **How:** Standard pattern: `platform_set_drvdata()` in probe and `platform_get_drvdata()` in remove.

### Remove()
```c
static void sai_led_remove(struct platform_device *pdev)
{
    pr_info("sai_led: remove called\n");
}
```
- **What:** Called when device is removed/unbound; release resources if you used non-devm allocations.
- **Why:** With `devm_` APIs you often don't need to explicitly free resources; still provide remove to log or do additional cleanup if needed.
- **How:** Note the `void` return type in modern kernels.

### OF match table + MODULE_DEVICE_TABLE
```c
static const struct of_device_id sai_led_of_table[] = {
    { .compatible = "sai,my-led" },
    { }
};
MODULE_DEVICE_TABLE(of, sai_led_of_table);
```
- **What:** The table of DT-compatible strings this driver supports. `MODULE_DEVICE_TABLE` generates module alias information used by depmod/modprobe/udev for auto-loading.
- **Why:** Without this, auto-modprobe of the module based on DT compatible won't occur.
- **How:** Kernel build system uses this to embed aliases like `of:N*T*C...sai,my-led` into the module.

### Platform driver struct and registration
```c
static struct platform_driver sai_led_driver = {
    .probe = sai_led_probe,
    .remove = sai_led_remove,
    .driver = {
        .name = "sai_led",
        .of_match_table = sai_led_of_table,
    },
};

module_platform_driver(sai_led_driver);
```
- **What:** Describe the platform driver (probe/remove + driver metadata) and register it with the kernel.
- **Why:** `module_platform_driver()` expands to module init/exit code that registers/unregisters the driver.
- **How:** When kernel finds platform devices whose DT node compatible matches the driver's `of_match_table`, it calls `probe()`.

### Module metadata
```c
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sai");
MODULE_DESCRIPTION("Simple LED driver using Device Tree + Platform driver");
```
- **What:** Module metadata.
- **Why:** License is important; non-GPL modules may taint kernel. Author/description help identify module.
- **How:** Keep GPL for kernel modules that use GPL-only symbols.

# 5. Build driver — Makefile & command

**Makefile:**
```makefile
obj-m := sai_led_driver.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
```

- `obj-m`: lists kernel objs to build as modules.
- `make -C`: tells the kernel build system where to compile; `M=$(PWD)` points to the external module directory.
- `modules`: builds `.ko`.
- `clean`: removes build artifacts.

**Build:**
```bash
make
```
*Result: `sai_led_driver.ko`*

# 6. Load and test (exact commands & explanation)

**Manual test (load module):**
```bash
sudo insmod sai_led_driver.ko
```
*`insmod` inserts the .ko into the running kernel. It does not consult module aliases; it simply loads the file.*

**Check logs:**
```bash
dmesg | tail -n 50
# or
dmesg | grep sai
```

**Look for probe messages:**
```
sai_led: probe called
sai_led: default-state=on
sai_led: probe success
```
*If present, driver bound & set LED.*

**Unload module:**
```bash
sudo rmmod sai_led_driver
```
*Removes module.*

# 7. Make module auto-load at boot (install into `/lib/modules`)

**To allow kernel auto-loading using modalias:**
```bash
sudo mkdir -p /lib/modules/$(uname -r)/extra/sai
sudo cp sai_led_driver.ko /lib/modules/$(uname -r)/extra/sai/
sudo depmod -a
```
*`depmod -a` computes module dependency/alias database used by modprobe. After this, loading with `modprobe sai_led` either manually or automatically (udev at boot) will cause proper binding.*

**Test auto load (without reboot):**
```bash
sudo rmmod sai_led_driver   # if loaded
sudo modprobe sai_led
dmesg | grep sai
```
*`modprobe` respects module aliases exported by `MODULE_DEVICE_TABLE` and can auto-bind to matching devices created by DT.*

**Reboot test:**
- Ensure overlay line present in `/boot/firmware/config.txt`
- Reboot
- `dmesg | grep sai` should show probe logs and LED active.

# 8. Troubleshooting (common issues, and how to solve them)

### Overlay compiled but node missing after reboot
- Check you put `.dtbo` into `/boot/overlays/` and added `dtoverlay=sai-led` to the correct config file (`/boot/config.txt` or `/boot/firmware/config.txt`).
- Verify with `dtc -I fs /sys/firmware/devicetree/base | grep -A5 sai-led`.

### Error: missing #gpio-cells or phandle warning
- That happens when you attached node under wrong parent (e.g., under the gpio controller). Use `target-path = "/"` to add node at root or attach correctly.

### probe() not called after module load
- If you used `insmod`, the driver may load but kernel won't match via modalias. Install module with `depmod -a` and use `modprobe` or ensure module is installed under `/lib/modules` so auto-binding can work.

### Continuous interrupts or flicker (if later using IRQ)
- Floating pin: give proper pull-down/up resistor. For mechanical buttons add debounce.

### Compilation errors referencing remove() signature
- Modern kernels (6.x) use `void remove(struct platform_device *)`. Adjust return type accordingly.

### "taints kernel" message on insmod
- Means out-of-tree module loaded. Normal during development.

# 9. Small examples / variations

### Example: change gpios to active-low

If your LED is wired in a way that active state is low, change DTS:
```dts
gpios = <&gpio 17 1>; /* 1 = active low convention in some bindings; numeric flags can vary per platform... confirm */
```
And in driver you could invert logic if desired.

### Example: multiple LEDs (child nodes)

**DTS:**
```dts
leds {
    compatible = "sai,multi-leds";
    led0 {
        compatible = "sai,my-led";
        gpios = <&gpio 17 0>;
        default-state = "on";
    };
    led1 {
        compatible = "sai,my-led";
        gpios = <&gpio 27 0>;
        default-state = "off";
    };
};
```
*Driver then iterates child nodes and registers per-led instances.*

# TO disable this module from autoloading :

## ✅ Option 1 — Disable the Device Tree Overlay (recommended)

Your driver autoloads because the overlay creates this DT node:
```dts
compatible = "sai,my-led";
```
which triggers module auto-loading.

So just comment out or remove the overlay entry.

**Edit the active config file:**
```bash
sudo nano /boot/firmware/config.txt   # (Bookworm/Trixie)
```

**Find:**
```ini
dtoverlay=sai-led
```

**Comment it:**
```ini
# dtoverlay=sai-led
```
Or delete the line.

**Reboot:**
```bash
sudo reboot
```

- ✔ DT node will not exist
- ✔ Driver will not auto-load
- ✔ LED will no longer be controlled by the driver

## ✅ Option 2 — Blacklist the module (prevents modprobe autoload)

If you want DT overlay to stay, but the driver should never load, blacklist it.

**Create a blacklist file:**
```bash
echo "blacklist sai_led_driver" | sudo tee /etc/modprobe.d/sai_led_blacklist.conf
```

**Rebuild module dependency database:**
```bash
sudo depmod -a
```

**Reboot:**
```bash
sudo reboot
```

- ✔ The module will not load even if a DT node matches
- ✔ You can still load manually with `insmod` (blacklist affects modprobe/udev only)

## ✅ Option 3 — Remove the installed .ko from `/lib/modules`

If you want a clean uninstall:
```bash
sudo rm /lib/modules/$(uname -r)/extra/sai/sai_led_driver.ko
sudo depmod -a
```

**Reboot.**
Since the module is gone, it cannot autoload.

> [!CAUTION]
> **Important Note**
> - `insmod sai_led_driver.ko` **ignores** blacklists
> - `modprobe sai_led` **respects** blacklists
>
> So:
> - If you blacklist → autoload prevented
> - But you can still manually load with `insmod`