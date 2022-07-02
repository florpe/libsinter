
# The FUSE protocol documentation

## Files

The `protocol.json` file, created using `generate.sh`, aggregates the documentation of all available versions as described in the subdirectories.

Each subdirectory contains the following files:

- `headers.json` : Describes the RX and TX headers
- `opcodes.json` : Maps operation names to opcodes
- `operations.json` : Maps operation names to information about that operation
- `structs.json` : Maps struct names to struct definitions

In the future an additional `structs.json` file might make the documentation more compact, and a `flags.json` file might provide some information about FUSE's flags and their meaning.

## Operations

Each entry in `operations.json` has the following fields:

- `request` : Describes the fields of the RX struct.
- `response` : Describes the fields of the TX struct. A null value means no reply message should be sent.

In the future additional keys might provide some commentary and flag information.

## Structs

The structs used by FUSE are 64-bit aligned, and variable-sized fields are confined to the end of the struct. Each entry is a field name mapped to a description of that field. The description consists of the following:

- `size` : Field size, in bits. A null value means variable size. Always present.
- `offset`: Field offset relative to the start of the struct or, for variable-sized fields, the start of the variable-sized fields section. Always present.
- `padding` : Whether the field's contents are padding and can be ignored. Defaults to false.
- `signed` : Whether the field represents a signed integer. Defaults to false.
- `cstringposition` : Indicates that the field is a null-terminated string and indicates its position in the variable-sized fields section, with zero indicating the first null-terminated string. A variable-sized field without this key comes after any null-terminated strings and extends to the end of the message.
- `struct` : Gives the name of a struct definition that applies to this field. Struct fields are guaranteed to only contain fixed-size fields.

The order of the fields is given by `(field["offset"], field.get("cstringposition", MAX\_INT))`. An empty description indicates that the reply should consist only of the mandatory header. A value of `-1` indicates missing information.

In the future the following additional field might be added:

- `flags` : To indicate that the field should be interpreted according to the corresponding entry in the not-yet-existing `flags.json` file.

## Testing

Not yet. I'm working on pysinter, a pure-Python FUSE library, which will be able to dynamically generate a FUSE interface from this documentation. To test this documentation and to serve as an example for pysinter I should write a passthrough file system.

## TODO

- Write that pysinter passthrough file system
- Incorporate flag information
- Incorporate usage information
- Add additional versions

## Acknowledgements

- hanwen/go-fuse
- libfuse/libfuse
- The libfuse Wiki with its protocol sketch


