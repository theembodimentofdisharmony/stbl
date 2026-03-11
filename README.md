# stbdl (Simple Taskbar Decoration Library)

Document: https://github.com/enstarep/rchp/blob/main/stbdl/README.md

Run the following command to install stbdl:

```
pip install stbdl
```
No dependencies need to be installed.

stbdl supports Python 3.x on Windows

## Latest Version Changes:
1. Release 0.1.0 version

---

# stbdl (Simple Taskbar Decoration Library)

Run the following command to install stbdl:

```
> pip install stbdl
```

No dependencies need to be installed.

stbdl supports Python 3.x on Windows 10+.

stbdl is a simple taskbar decoration library.

It mainly includes the following four functions:

```python
def window(path: Optional[str] = None, mode: int = 3) -> None: ...
def expansion() -> None: ...
def taskbar_switcher() -> wintypes.HWND: ...
def border(hwnd: wintypes.HWND) -> None: ...
```

These functions are all under the `stbdl.*` namespace.

## `window` function

The parameter `path` is the path to the background image, **must be an absolute path**. If no value is provided, it defaults to loading `stbdl\white.png`.  
The parameter `mode` is the dynamic style, with the following meanings:
```text
mode=0: Fade in/out
mode=1: Rotation
mode=2: Zoom
mode=3: Four-color cycle (red, green, blue, yellow)
mode=4: Color cycle
```

If no value is provided, it defaults to 3.

Also, the background window needs to be clicked on a blank area of the taskbar to bring the taskbar above it.

The `window` function returns no value.

**Tip: Do not run the window function multiple times, as this will create multiple stbdl processes (but they can be terminated using `taskkill /im stbdl.exe`)**

## `expansion` function

Enables the stbdl taskbar extension. The actual effect depends on system support. It prioritizes the mica effect (requires Windows 11), then the acrylic effect (requires Windows 10), and finally the blur effect.

The `expansion` function takes no parameters and returns no value.

## `taskbar_switcher` function

Gets the handle of the task switcher window (class name `MSTaskListWClass`). Returns None if not found.

The `taskbar_switcher` function takes no parameters.

## `border` function

Used to add a border to a specified window. It takes the handle of the target window as a parameter.

The `border` function returns no value.

## Example

```python
import stbdl

stbdl.window()

stbdl.expansion()

h = stbdl.taskbar_switcher()

stbdl.border(h)
```
