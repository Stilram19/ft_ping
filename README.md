
# ft_ping

`ft_ping` is a custom implementation of the `ping` utility, faithfully recreating the behavior of the `inetutils-2.0` version of GNU ping. This project was undertaken to deepen understanding of network programming, raw sockets, and the ICMP protocol.

It intelligently creates sockets to allow execution by unprivileged users and features a highly efficient ping loop that uses `select()` for productive waiting instead of a simple `sleep`.

## Features

*   **ICMP Echo Requests**: Sends ICMP `ECHO_REQUEST` packets to a specified network host.
*   **Standard Ping Options**: Supports common `ping` options like `-v` (verbose), `-q` (quiet), `-c` (count), `-i` (interval), and `-s` (size).
*   **Flood Ping**: Includes a `-f` (flood) option to send packets as fast as possible.
*   **Privilege Fallback**: Attempts to use `SOCK_RAW` and falls back to `SOCK_DGRAM` on permission failure, allowing the program to run without root privileges in many modern Linux environments.
*   **Detailed Statistics**: Provides a summary of packet transmission, reception, and round-trip times.

## How to Build and Run

### Prerequisites

*   A C compiler (like `gcc` or `clang`)
*   `make`

### Building

To build the project, simply run `make`:

```sh
make
```

This will create the `ft_ping` executable in the root directory.

### Usage

```sh
./ft_ping [options] <host>
```

**Options:**

| Option | Description                                                                                                 |
|--------|-------------------------------------------------------------------------------------------------------------|
| `-v`   | Verbose output.                                                                                             |
| `-q`   | Quiet output. Nothing is displayed except summary lines.                                                    |
| `-c count` | Stop after sending `count` ECHO_REQUEST packets.                                                        |
| `-i wait`  | Wait `wait` seconds between sending each packet.                                                        |
| `-s size`  | Specify the number of data bytes to be sent. The default is 56.                                         |
| `-f`   | Flood ping. Send packets as fast as they come back or one hundred times per second, whichever is more.      |
| `-h`   | Show help message and exit.                                                                                 |

**Example:**

```sh
# Ping google.com 5 times
./ft_ping -c 5 google.com

# Flood ping localhost
sudo ./ft_ping -f localhost
```
Note: Flood mode is best run with `sudo` to use `SOCK_RAW` and avoid rate-limiting that might apply to unprivileged ICMP sockets.
