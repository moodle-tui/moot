# moot
Moot (Moodle TUI) is a terminal interface application for interacting with [Moodle](https://moodle.org/) learning management system.

---
⚠️ Caution - still work in progress ⚠️

---

## Features
As of now, the functionality is focused for students to upload their assignments
and download files from the server. Currently only `topics` and `weeks` format
courses are supported. At least partially supported modules:
 - `Assignment` - description, attached files and file submission.
 - `Workshop` - description and file submission.
 - `Resource` - description and attached files.
 - `Url` - description and the actual url.

## Usage

### Authentication

To use the tool you will need to acquire your personal Moodle token. This can
only be done if the Moodle site has enabled the [mobile
app](https://github.com/moodlehq/moodleapp). There are two ways to get your
personal token:
1. **If the Moodle site uses simple site login**, you can simply go to the
   following url:
   `https://<moodle_site>/login/token.php?username=<your_username>&password=<your_pass>&service=moodle_mobile_app`

2. **If the Moodle site uses some other auth system**, you should login to the
   site in a browser and then go to the following url:
   `https://<moodle_site>/admin/tool/mobile/launch.php?service=moodle_mobile_app&passport=208465373836275&urlscheme=moodlemobile`.
   You will be redirected to custom url scheme which includes the text
   `moodlemobile://token=<token_base64>`. If the browser does not display this,
   (e. g. firefox shows *The address wasn’t understood* error), you should open
   developer tools in the browser before making the request and look for the
   `Location` response header, which will have the required text.
 
   Now you need to [base64 decode](https://www.base64decode.org/) the
   <token_base64> part. The output will look like this:
   `<first_part>:::<second_part>`, with the `<second_part>` being the token you
   actually need.
   
Token should to be specified in config file. See [Configuration](https://github.com/moodle-tui/moot#configuration)

### Interface
To use the app simply run the executable. 
You will be presented with [lf](https://github.com/gokcehan/lf) like interface,
where the middle column is the current column you are navigating in.

### Commands
These are commands, that moot interface currently supports. More comming soon.
- To move around, us the arrow keys or vim equivalents (`h`, `j`, `k`, `l`).
- To quit, press `q`.
- To download a file, hover it and press `s`.
- To upload files, hover desired module and press `u`. (Only assignments currently supported)

## Installing
Currently we don't provide any prebuilt binaries, so one has to build for himself and put the final executable in [path](https://en.wikipedia.org/wiki/PATH_(variable))

### Compiling

#### Operating system
While it has only been built and tested on linux, it is coded with portability and being cross-platform in mind, so it should be possible to build on MAC OS or Windows.

#### Requirements
 - GNU make
 - C compiler (at least C11)
 - [libcurl](https://curl.se/libcurl/)
 - [lf](https://github.com/gokcehan/lf) (for files selection. Optional)

#### Configuring
Compiling (compiler, target executable, etc) can be configured for your system
in the `config.mk` file.

### Configuration
App can be configured through config file, that should be located at `$XDG_CONFIG_HOME/moot/config` on unix systems and `%LOCALAPPDATA%\moot\config` on windows systems.

Format: `<property> = <value>`

Currently supported properties:
- `token` (required).
    Should be equal to your personal moodle token.
- `upload_command`.
    Should return newline seperated file paths to stdout.

#### Building

run `make` from the root directory.

## Licence and copyright
https://github.com/moodle-tui/moot

See LICENCE file   
