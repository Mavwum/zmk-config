# Keebart ZMK Firmware

This repository contains the ZMK firmware configuration for Keebart wireless
split keyboards. It includes board definitions, default keymaps, ZMK Studio
support, Sharp Memory-in-Pixel display support, RGB underglow, and GitHub
Actions firmware builds.

- Maintainer: [Keebart](https://github.com/Keebart)
- Firmware: [ZMK](https://zmk.dev/)
- Miryoku firmware: [Keebart/miryoku_zmk](https://github.com/Keebart/miryoku_zmk)
- Online keymap editor: [ZMK Keymap Editor](https://nickcoutsos.github.io/keymap-editor/)
- Runtime editor: [ZMK Studio](https://zmk.studio/)

## Supported keyboards

| Keyboard | Layout | Board targets | Bluetooth name |
| --- | --- | --- | --- |
| [Corne Choc Pro BT](https://keebart.com/products/corne-wireless) | 6-column | `corne_choc_pro_left`, `corne_choc_pro_right` | `Corne Choc BT` |
| Corne Choc Pro BT 5-Col | 5-column | `corne_choc_pro_5col_left`, `corne_choc_pro_5col_right` | `Corne Choc BT` |
| [Piantor Pro BT](https://keebart.com/products/piantor-wireless) | 6-column | `piantor_pro_bt_left`, `piantor_pro_bt_right` | `Piantor Pro BT` |
| Piantor Pro BT 5-Col | 5-column | `piantor_pro_bt_5col_left`, `piantor_pro_bt_5col_right` | `Piantor Pro BT` |
| [Sofle Choc Pro BT](https://keebart.com/products/sofle-wireless) | 6-column | `sofle_choc_pro_left`, `sofle_choc_pro_right` | `Sofle Choc BT` |

The shortened Bluetooth names fit ZMK's 16-character device-name limit while
making wireless models identifiable in the host's Bluetooth menu.

## Firmware variants

Corne and Piantor have separate 6-column and 5-column firmware targets. The
6-column targets retain the standard keymaps. Targets ending in `_5col` use a
compact keymap designed specifically for boards without the outer columns.

Separate firmware is intentional. ZMK Studio can switch between physical
layouts in one firmware, but it stores one runtime keymap rather than two
independent defaults. Separate targets also give the ZMK Keymap Editor one
unambiguous source keymap and geometry for each keyboard variant.

Select matching firmware for both halves. Do not mix a regular target with an
`_5col` target.

## Default 5-column keymap

The compact keymap uses home-row modifiers and thumb keys that tap common keys
or hold dedicated layers.

### Home-row modifiers

Tap a home-row key to type its letter. Hold it to use its modifier.

| Key | Hold action | Key | Hold action |
| --- | --- | --- | --- |
| `A` | Left GUI | `J` | Right Shift |
| `S` | Left Alt | `K` | Right Ctrl |
| `D` | Left Ctrl | `L` | Right Alt |
| `F` | Left Shift | `'` | Right GUI |

The mod-taps use a 200 ms tapping term and ZMK's `tap-preferred` flavor to avoid
turning ordinary same-hand typing rolls into modifiers.

### Thumb keys

| Tap | Hold |
| --- | --- |
| `Esc` | Media layer |
| `Space` | Navigation layer |
| `Tab` | Function layer |
| `Enter` | Symbol layer |
| `Backspace` | Number layer |
| `Delete` | Adjust layer |

### Layers

| Layer | Main contents |
| --- | --- |
| `QWERTY` | Letters, punctuation, home-row modifiers, and layer-tap thumbs |
| `NAV` | Arrow keys, Home, End, Page Up/Down, Insert, Caps Lock, and modifiers |
| `MEDIA` | Playback, volume, and RGB controls including hue down/up |
| `NUM` | Numpad-style digits, brackets, semicolon, equals, backslash, and grave |
| `SYM` | Braces, parentheses, operators, colon, and shifted symbols |
| `FUN` | F1-F12, Print Screen, Scroll Lock, Pause, and modifiers |
| `ADJUST` | Bluetooth profiles, RGB controls, reset, bootloader, and Studio unlock |

To unlock ZMK Studio on a 5-column build, hold the `Delete` thumb key and press
`C`. Reset and bootloader are on `Z` and `X` on the same layer.

The source files are:

- `config/corne_choc_pro_5col.keymap`
- `config/piantor_pro_bt_5col.keymap`

Matching board-default copies are kept under `boards/arm/*_5col/`.

## Miryoku firmware

Alternative firmware using the [Miryoku](https://github.com/manna-harbour/miryoku)
layout is maintained in
[Keebart/miryoku_zmk](https://github.com/Keebart/miryoku_zmk). Its GitHub Actions
workflows build Miryoku firmware for the standard Corne Choc Pro BT, Piantor
Pro BT, and Sofle Choc Pro BT targets.

The Miryoku repository loads this repository as an external ZMK module. Board
definitions and the `sharp_mip` display shield remain here, while Miryoku owns
the keymap and physical mappings. Its keyboard workflows select `sharp_mip` as
an extra shield, so the custom display remains available in Miryoku firmware.

Run the matching workflow from the Miryoku repository's **Actions** tab and
flash the generated left and right firmware files to their corresponding
halves. Miryoku firmware is an alternative to the default keymaps built by
this repository; it is not an additional runtime-selectable keymap.

## Building firmware

The GitHub Actions workflow builds all targets on every push and pull request.
It can also be started manually from the repository's **Actions** tab. Download
the merged `firmware` artifact after the build finishes.

The build matrix is defined in `build.yaml`. Normal firmware uses the custom
`sharp_mip` shield on every keyboard. Corne and Piantor use its default
orientation; Sofle enables the 180-degree rotation in its board configuration.
The central, left-side builds include ZMK Studio support.

For a local build, first create a normal ZMK west workspace using the manifest
in `config/west.yml`. From the workspace root, run a command such as:

```sh
west build -s zmk/app -d build/corne-5col-left \
  -b corne_choc_pro_5col_left \
  -S studio-rpc-usb-uart -- \
  -DZMK_CONFIG=/path/to/zmk-config/config \
  -DZMK_EXTRA_MODULES=/path/to/zmk-config \
  -DSHIELD=sharp_mip \
  -DCONFIG_ZMK_STUDIO=y
```

Change the board target as required. The resulting firmware is written to
`build/corne-5col-left/zephyr/zmk.uf2`.

## Flashing

The halves use different firmware because the left half is the central side
and the right half is the peripheral side.

1. Download or build the firmware package.
2. Enter the bootloader on the right half and copy its matching right-side UF2
   file to the USB mass-storage device.
3. Enter the bootloader on the left half and copy its matching left-side UF2
   file.
4. Reconnect the keyboard and pair it with the host if necessary.

Enter the bootloader by double-pressing the physical reset button, or use the
bootloader key in the active keymap. On the 5-column keymap, hold `Delete` and
press `X`.

### Resetting saved settings

The build artifact also contains `settings_reset` firmware for the standard
Corne, Piantor, and Sofle targets. Use it when split pairing or stored settings
prevent normal operation:

The standard Corne and Piantor settings-reset files can also be used before
reflashing their matching 5-column firmware because the hardware is the same.

1. Flash the appropriate settings-reset firmware to a half.
2. Allow it to boot and clear the saved settings.
3. Immediately flash the normal firmware for that half again.
4. Repeat for the other half when resetting split pairing.

Settings-reset firmware is temporary and is not a usable keyboard firmware.

## Editing keymaps

### ZMK Keymap Editor

The [ZMK Keymap Editor](https://nickcoutsos.github.io/keymap-editor/) edits the
source `.keymap` files in this repository. Choose the file matching the
keyboard and column count. The accompanying JSON files in `config/` provide
the correct visual geometry.

Editing and committing a source keymap triggers a new GitHub Actions build.
This is the recommended workflow for maintaining and distributing defaults.

### ZMK Studio

[ZMK Studio](https://zmk.studio/) edits the runtime keymap stored on the
keyboard. Connect the left half directly by USB, unlock Studio from the
keymap, and select the device in the browser.

Studio changes do not update the `.keymap` source file. Conversely, flashing
new firmware may not replace a Studio-edited keymap because the runtime state
is saved. Use **Restore Stock Settings** in Studio after changing between
6-column and 5-column firmware or whenever the compiled default should be
loaded again.

## Power management

Deep sleep is enabled on every half. The keyboard enters deep sleep after one
hour (`3600000` ms) without activity and wakes when a key is pressed. The long
timeout avoids the keyboard appearing to sleep during normal breaks while
still conserving battery during extended inactivity.

## RGB controls

RGB underglow is enabled with a conservative maximum brightness. On the
5-column keymap, hold `Esc` for the Media layer:

- `Q`: hue down (`RGB_HUD`)
- `W`: hue up (`RGB_HUI`)
- `E` / `R`: saturation down/up
- `T`: cycle effect

Brightness and toggle controls are on the bottom row of the same layer. The
Adjust layer also contains the complete RGB control set for testing.

## Rotary encoders

Corne Choc Pro supports four encoder definitions:

1. Volume down/up
2. Page Up/Down
3. Previous/next media track
4. Volume down/up

Sofle Choc Pro uses one encoder for volume and the other for previous/next
media track. The encoder actions are available on its default, Lower, and
Raise layers.
