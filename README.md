# DelHelX

`DelHelX` helps VATSIM controllers by automating delivery related checks in EuroScope.

## Table of Contents

-   [Getting started](#getting-started)
    -   [Prerequisites](#prerequisites)
    -   [Installation](#installation)
-   [Usage](#usage)
    -   [Basics](#basics)
    -   [Tag items](#tag-items)
    -   [Tag functions](#tag-functions)
    -   [Chat commands](#chat-commands)
-   [Contributing](#contributing)
    -   [Development setup](#development-setup)
-   [License](#license)

## Getting started

### Prerequisites

Since `DelHel` was developed as an EuroScope plugin, it requires a working installation [EuroScope](https://euroscope.hu/). The initial development was started using EuroScope version [`v3.2.9`](https://www.euroscope.hu/wp/2020/06/28/v3-2-1-25/), although the plugin should most likely also work fine with previous and later versions. As development continues, compatibility to the latest **beta** versions of EuroScope will be maintained as long as possible and feasible.

### Installation

TODO update screenshots

1. Download the latest release (`DelHelX.zip`) of `DelHelX` from the [**Releases**](https://github.com/sushiat/DelHelX/releases/latest) section of this repository
2. Extract `DelHelX.dll` and place it into your plugin directory (most likely somewhere inside your EuroScope sectorfile/profile setup, where other plugins are already set up)
3. Start EuroScope and open the **Plug-ins** dialog in the settings menu (**OTHER SET**)
   ![Plug-ins dialog](https://i.imgur.com/SrVtRp9.png)
4. **Load** the plugin by selecting the `DelHelX.dll` you extracted and ensure the proper version is displayed
   ![Load plugin](https://i.imgur.com/y6koC4g.png)
   `DelHelX` will also confirm successful initialisation by logging its version to the **Messages** chat:
   `[08:34:10] DelHelX: Version 0.1.0 loaded.`
5. Close the plugin dialog and open the startup list columns setup dialog (small **S** at the left side of your startup list)
   ![Departure list columns setup dialog](https://i.imgur.com/MvFYkkh.png)
6. (_Optional_) Add the **Flightplan Validation** column to your departure list by clicking **Add Item** and selecting the `DelHel / Flightplan Validation` **Tag Item type**. Pick a **Header name** and set a **Width** of 4 or greater. This column will display warnings and the status of each flightplan processed by DelHel, but is not strictly required for the plugin to function
7. Assign the `DelHel / Process FPL` action as the **Left button** or **Right button** action of any of your tag items as desired. Triggering this function processes the selected flightplan using the default settings of `DelHel` (described in more detail in the [Process FPL](#process-fpl) section below)
8. (_Optional_) Assign the `DelHel / Validation menu` action as the **Left button** or **Right button** action of any of your tag items as desired. Triggering this function opens the flightplan validation menu, allowing for more fine-grained processing of the selected flightplan (described in more detail in the [Validation Menu](#validation-menu) section below)
9. Close the departure list settings by clicking **OK**

## Usage

The following indications are available:

TODO describe

##### `OK`

Info, green color.  
Indicates a flightplan has been processed and no validation errors or warnings have been found.

### Tag functions

Tag functions are used to trigger plugin functionality via a flightplan tag in aircraft lists, such as the departure or arrival list.  
At the moment, `DelHelX` adds only a single one

#### Release PS

TODO describe this


### Chat commands

Chat commands allow more fine-grained control of `DelHelX`'s behavior and settings not available via UI elements. Every chat command is prefixed with `.delhelx` and can be entered in every chat channel available. Executing `.delhel` without any additional commands prints the version loaded and a list of commands available.

#### Toggle debug logging

`.delhelx debug`

Toggles debug logging, displaying more messages about the internal state and flightplan processing.

This setting will be saved to the EuroScope settings upon exit.

#### Toggle update checks

`.delhelx update`

Toggles the plugin update check upon EuroScope startup.

If enabled, `DelHelX` will check this repository for newer releases of the plugin, displaying a message should an update be available.

This setting will be saved to the EuroScope settings upon exit.


#### Toggle flashing of DelHel messages

`.delhelx flash`

Toggles flashing of unread message indicator for messages in the DelHel group. Note that, once disabled, all `DelHelX` messages will continue to flash until you have restarted EuroScope (saving your plugin settings). This unfortunately seems to be a EuroScope limitation we cannot work around.

If enabled, messages sent to the `DelHelX` group will have a flashing unread indiciator.  
If disabled (default setting), unread messages will only light up once, solidly (only applies after EuroScope has been restarted).

This setting will be saved to the EuroScope settings upon exit.

## Contributing

If you have a suggestion for the project or encountered an error, please open an [issue](https://github.com/sushiat/DelHelX/issues) on GitHub. Please provide a summary of your idea or problem, optionally with some logs or screenshots and ways to replicate for the latter.  

[Pull requests](https://github.com/sushiat/DelHelX/pulls) are highly welcome, feel free to extend the plugin's functionality as you see fit and submit a request to this repository to make your changes available to everyone. Please keep in mind this plugin attempts to provide features in a relatively generic way so it can be used by vACCs with different needs - try refraining from "hard-coding" any features that might just apply to a specific airport or vACC.
This is currently a big TODO item in the initial release version of DelHelX, it's currently pretty much hard-coded to be used in LOWW only.

### Development setup

`DelHelX` currently has no external development dependencies aside [Visual Studio](https://visualstudio.microsoft.com/vs/). Initial development started using Visual Studio 2022, although later versions should most likely remain compatible.

To allow for debugging, the project has been configured to launch EuroScope as its debug command. Since your installation path of EuroScope will most likely be different, you **must** set an environment variable `EUROSCOPE_ROOT` to the **directory** EuroScope is installed in (**not** the actual `EuroScope.exe` executable), for instance `E:\EuroScope`.  
Note: triggering a breakpoint seems to cause both EuroScope and Visual Studio to freak out, resulting in high resource usage and slugging mouse movements, thus only being of limited usefulnes. **NEVER** debug your EuroScope plugin using a live connection as halting EuroScope apparently messes with the VATSIM data feed under certain circumstances.

`DelHelX` is compiled using Windows SDK Version 11.0 with a platform toolset for Visual Studio 2022 using the ISO C++17 Standard.

This repository contains all external dependencies used by the project in their respective `include` and `lib` folders:

-   `EuroScope`: EuroScope plugin library
-   `nlohmann/json`: [JSON for Modern C++](https://github.com/nlohmann/json/) ([v3.9.1](https://github.com/nlohmann/json/releases/tag/v3.9.1), [MIT License](https://github.com/nlohmann/json/blob/develop/LICENSE.MIT)), used for parsing the airport config JSON
-   `semver`: [Semantic Versioning C++](https://github.com/Neargye/semver) ([v0.2.2](https://github.com/Neargye/semver/releases/tag/v0.2.2), [MIT License](https://github.com/Neargye/semver/blob/master/LICENSE)), used for version comparison of update check

## License

[MIT License](LICENSE)
