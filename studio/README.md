# Keebart Studio firmware

Studio firmware supports Corne Choc Pro BT, Sofle Choc Pro BT and Piantor Pro BT.
The complete firmware sources and build configuration are in this repository.
Builds use upstream ZMK v0.3 and the dependencies in `config/west.yml`; they do
not require the Keebart Studio app source or any private repository.

## Build with GitHub Actions

Fork this repository, edit the matching `config/*.conf` file, and push your
changes. **Actions → Build ZMK firmware** builds the targets in `build.yaml`.
Download the `firmware` artifact and select the matching left and right files.
The six-column `*_keebart_left` / `*_keebart_right` images use Keebart Studio.
Other entries retain their own firmware configuration.

## Build locally

Set up the ZMK v0.3 toolchain and a West workspace using `config/west.yml`.
From that workspace, build Corne Choc Pro BT with:

```sh
west build -s zmk/app -d build/corne-left \
  -b corne_choc_pro_left -S keebart-studio -- \
  -DZMK_CONFIG=/path/to/zmk-config/config \
  -DZMK_EXTRA_MODULES=/path/to/zmk-config \
  -DSHIELD=sharp_mip

west build -s zmk/app -d build/corne-right \
  -b corne_choc_pro_right -S keebart-studio-peripheral -- \
  -DZMK_CONFIG=/path/to/zmk-config/config \
  -DZMK_EXTRA_MODULES=/path/to/zmk-config \
  -DSHIELD=sharp_mip
```

For another keyboard, use these board names and the corresponding left snippet:

| Keyboard | Left board | Right board | Left snippet |
| --- | --- | --- | --- |
| Corne Choc Pro BT | `corne_choc_pro_left` | `corne_choc_pro_right` | `keebart-studio` |
| Sofle Choc Pro BT | `sofle_choc_pro_left` | `sofle_choc_pro_right` | `keebart-studio-sofle-bt` |
| Piantor Pro BT | `piantor_pro_bt_left` | `piantor_pro_bt_right` | `keebart-studio-piantor-bt` |

All right boards use `keebart-studio-peripheral`. Each image is written to
`<build-directory>/zephyr/zmk.uf2`.

## Customize

- `config/corne_choc_pro.conf`, `config/sofle_choc_pro.conf` and
  `config/piantor_pro_bt.conf`: user firmware preferences, including idle,
  power and display settings.
- `snippets/keebart-studio*/`: model and role integration settings.
- `studio/stock_*.h`: Studio's factory keymap bindings. Studio builds use these
  defaults and the runtime Studio keymap overlay; ordinary ZMK `.keymap` edits
  do not replace the Studio bindings.
- `studio/lighting_defaults.h`: Studio's factory lighting defaults.
- `boards/`: hardware definitions.

Existing saved Studio settings take precedence over factory defaults. Runtime
keymaps, lighting, macros, combos and other assignments can be edited and saved
with Keebart Studio. Keyboard names saved there also become the USB/Bluetooth
name. Normal Studio updates preserve settings and Bluetooth pairing.
