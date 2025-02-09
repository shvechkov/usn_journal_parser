# USN Parser

This is a command-line tool for parsing and analyzing the USN (Update Sequence Number) Journal on Windows systems. The USN Journal is a feature of the NTFS file system that records changes to files and directories on a volume.
Unlike fsutil, this tool provides more detailed information about the USN Journal records, including Full file name. 

## Usage
Where `cmd` is one of the following:

- `help`: Display the help message.
- `stats`: Get the USN Journal statistics.
  - Example: `usn_parser stats --volume "\\\\.\\C:"`
- `usn_last`: Get the last USN number.
- `usn_read`: Read USN Journal records.
  - Example: `usn_parser usn_read --volume "\\\\.\\C:" [--start <usn_start>] [[--end <usn_end>] | [--count <count>]]`

## Parameters

- `--volume <path>`: The path to the volume to analyze.
- `--start <usn_start>`: The starting USN number to read from.
- `--end <usn_end>`: The ending USN number to read until.
- `--reason <usn_reason>`: Filter records by the specified USN reason.
- `--regexp <regexp>`: Filter records by the specified regular expression.
- `--format <txt|csv|json>`: The output format for the parsed records.
- `--ofile <path>`: The output file to write the parsed records.

## Examples

- Get the USN Journal statistics:
  ```
  usn_parser stats --volume "\\\\.\\C:"
  ```
- Read USN Journal records from the specified USN number:
  ```
  usn_parser usn_read --volume "\\\\.\\C:" --start 0x0
  ```
- Read USN Journal records from the specified USN number to the end:
  ```
  usn_parser usn_read --volume "\\\\.\\C:" --start 0x0 --end 0x1000
  ```
- Read USN Journal records from the specified USN number with the specified count:
  ```
  usn_parser usn_read --volume "\\\\.\\C:" --start 0x0 --count 100
  ```
- Read USN Journal records from the specified USN number with the specified output format:
  ```
  usn_parser usn_read --volume "\\\\.\\C:" --start 0x0 --format csv
  ```
## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for more information.
