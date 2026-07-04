# CM (CapsLock Mapper)

A small utility that maps the Caps Lock key to a left mouse click to reduce wrist strain.

## Guides

* [Windows Version](./packages/windows/README.md)
* [Linux Version](./packages/linux/README.md)

## Building from Source

Both platforms build from the repository root with CMake (3.13+); the right
package is selected automatically:

```bash
cmake -S . -B build
cmake --build build
```

Binaries land in `build/packages/<platform>/`.

## License

This project is licensed under the MIT License. See the [LICENSE](./LICENSE) file for details.