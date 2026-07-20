# pkg-thermald

This is the Debian packaging overlay for the thermald project.
Upstream project is hosted here: https://github.com/intel/thermal_daemon

## About

This overlay package applies backported patches to the Debian thermald package
that have been upstreamed in thermald but are not yet included in the official
Debian package. These patches improve thermal management support on Qualcomm
platforms.

## Branches

- **qli-ci**: The primary branch containing workflow logic in the `.github/` folder, along with boilerplate documentation files such as license, contribution guidelines, and this README.
- **qcom/debian/latest**: An orphan starter branch shipping a `debian/` directory layout. It is **not** meant to be used as-is for a real package; see [Setting up the packaging branch](#setting-up-the-packaging-branch) for how to construct your own `qcom/debian/latest`. Naming conventions are documented [here](https://qualcomm-confluence.atlassian.net/wiki/spaces/LinuxCoreOS/pages/2879858691/pkg-+repository+specification).

## Installation Instructions

```
sudo dpkg -i thermald_*.deb
```

## Usage

Build: To build the package, go to the *Actions* tab, select the
*Build Debian Package* workflow, then click 'Run workflow'.

Upstream Version Promotion: To promote a new upstream version, go to the
*Actions* tab, select the *Upstream Version Promotion* workflow.

The workflows of this repo use the reusable workflows from qcom-build-utils
in the background. To understand more about how everything connects together,
see https://github.com/qualcomm-linux/qcom-build-utils


## Development

How to develop new features/fixes for the software. Maybe different than "usage". Also provide details on how to contribute via a [CONTRIBUTING.md](CONTRIBUTING.md)

## Getting in Contact

* [Report an Issue on GitHub](../../issues)
* [Open a Discussion on GitHub](../../discussions)
* [E-mail us](mailto:opensource@qualcomm.com) for general questions

## License

pkg-thermald is licensed under the [BSD-3-Clause License](https://spdx.org/licenses/BSD-3-Clause.html). See [LICENSE.txt](LICENSE.txt) for the full license text.
