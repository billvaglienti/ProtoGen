Introduction
============

ProtoGen is short for "protocol generation". ProtoGen owes its provenance to an often repeated problem: how to efficiently and in a cross platform way communicate information from one computer to another. This problem appears in many places, but most commonly in connecting microcontroller-based peripheral devices (GPSs, Autopilots, IMUs, etc) together. These devices maintain a sophisticated internal state representation in memory; and communicating this data efficiently and in a cross platform way is non-obvious. By efficient I mean: efficient in the amount of bandwidth used. By cross platform I mean: different computer architectures, memory order, compilers, and register bit widths should not cause problems for the communication.

The naive way of doing this is to copy the data in memory into a packet which is sent to the other computer. As an example consider a GPS: it outputs a three dimensional position estimate. Internally the position is represented as three double precision numbers (latitude, longitude, altitude, or perhaps ECEF x, y, z), occupying a total of 24 bytes of memory. When using LLA or ECEF the position number needs enough dynamic range to represent the position to a resolution which is better than the GPS accuracy. If we want ECEF position resolution of 0.01 meters, then we need approximately log2(7,000,000/0.01) ~ 30 bits. This is why double precision is required, since single precision only gives 23 bits of resolution.

If one were to simply `memcpy()` the double precision representation into a packet there are a number of negative consequences:

1. The number can fit in a dynamic range of 32-bits, but 64 bits are used; doubling the amount of bandwidth required.
2. The receiver may use a different byte order than the sender (little versus big endian).
3. The receiver may use different structure packing rules than the sender.
4. The receiver may have different memory alignment rules than the sender.
5. The receiver's definition of floating point numbers may be different from the sender. (To be fair, most systems now use [IEEE-754 for floating point](http://grouper.ieee.org/groups/754/), so this is unlikely).

These problems can be averted if the internal data representation is converted to a common representation as given by an interface control document (ICD) before transmission. The receiver then un-converts the received data according to the same ICD. The conversion can account for all of the problems of simple copying. Most peripheral devices use some version of this idea: the device comes with an ICD that defines how to interpret the transmitted data. Software is then written according to this ICD to implement the encoding/decoding rules. However this creates new problems:

1. Implementing communications now requires a lot more software development. You cannot just `memcpy()` to or from the packet data buffer.
2. CPU time is spent at both the receiver and transmitter to perform the ICD interpretation.
3. The communications protocol is typically written at least three times: the encode (transmit), the decode (receive), and the documentation (ICD). Keeping all three implementations in sync is challenging and a common source of bugs.

ProtoGen is a tool that takes a xml protocol description and generates html for documentation, and C source code for encoding and decoding the data. This alleviates much of the challenge and bugs in protocol development. The C source code is highly portable, readable, efficient, and well commented. It is suitable for inclusion in almost any C/C++ compiler environment.

This document refers to ProtoGen version 1.9.1. You can download the prebuilt versions for [windows, mac, and linux here](https://github.com/billvaglienti/ProtoGen/releases/download). Source code for ProtoGen is available on [github](https://github.com/billvaglienti/ProtoGen).

---

Usage
=====

ProtoGen is a C++/Qt5 compiled command line application, suitable for inclusion as a automated build step (Qt provides the xml, string, and file handling). The command line is: `ProtoGen Protocol.xml [Outputpath] [-docs dir] [-latex] [-no-doxygen] [-no-markdown] [-no-helper-files] [Style.css] [-no-unrecognized-warnings]`. On Mac OS protogen is invoked through an app bundle: `ProtoGen.app/Contents/MacOS/ProtoGen`

- `Protocol.xml` is the file that defines the protocol details.

- `Outputpath` is an optional parameter that gives the path where the generated files should be placed. If `Outputpath` is not given then the files will be placed in the working directory from which ProtoGen is run.

- `-docs dir` specifies a separate directory for the documentation markdown to be written. If `-docs dir` is not specified, documentation markdown will be written to the same same directory as `Outputpath`.

- `-show-hidden-items` will cause documentation to be generated for **all** elements, even if the element has the *hidden="true"* flag

- `-latex` will cause ProtoGen to generate LaTeX style markdown (if multimarkdown is installed and in the PATH).

- `-latex-header-level level` specifies the starting header-level for generated LaTeX output. If unspecified, it defaults to `1` (Chapter headings)

- `-no-doxygen` will cause ProtoGen to skip the output of the developer level html documentation. 

- `-no-markdown` will cause ProtoGen to skip the output of the user level html documentation. 

- `-no-helper-files` will cause ProtoGen to skip the output of files not directly specified by the protocol.xml. 

- `Style.css` will replace the default inline css in the markdown documentation with the contents of the Style.css file.

- `-no-unrecognized-warnings` will suppress warnings about unrecognized tags or attributes in the `Protocol.xml` file. This is useful if you add data to your xml that you expect Protogen to ignore. 


Dependencies
------------

ProtoGen contains the [Qt dependencies](http://qt-project.org/) in its distribution, so Qt does not need to be installed on the host system. ProtoGen outputs documentation for protocol users in [MultiMarkdown](http://fletcherpenney.net/multimarkdown/) format. The host system must have MultiMarkdown installed for ProtoGen to create the html output. On windows and linux MultiMarkdown is invoked simply as "multimarkdown". On mac it is invoked as "/usr/local/bin/MultiMarkdown".

The source code generated by ProtoGen also contains doxygen markup comments. If the host system has [doxygen](http://www.stack.nl/~dimitri/doxygen/index.html) installed ProtoGen can output html documentation for protocol developers. On windows and linux doxygen is invoked by ProtoGen as simply "doxygen". On mac doxygen is invoked by ProtoGen as "/Applications/Doxygen.app/Contents/Resources/doxygen"; this is because doxygen for the mac is provided as a .app bundle containing both the GUI and the binary.

Using ProtoGen as a compiler pre-build step
-------------------------------------------

ProtoGen will not touch an output file if the generated file is not different from what already exists. In this way you can run ProtoGen repeatedly without worrying about causing unneeded rebuild of your project. ProtoGen runs very quickly, and if you use the `-no-doxygen` switch it is typically fast enough to run it every time you compile. Note that the generated files include a comment with the ProtoGen version, so if you change ProtoGen version you will get updated output (and hence a compiler rebuild) even if the protocol code did not change.

ProtoGen applies many checks to the protocol xml. In most cases if a problem is discovered the protocol is altered as needed and ProtoGen will output a warning on stdout. The warnings conform to the layout most IDEs will expect; so you can click directly on the warning and go straight to the offending line in the xml (I've only tesed this in Qt Creator).

Protocol ICD
================

The protocol is defined in an [xml](http://www.w3.org/XML/) document. ProtoGen comes with an example document (exampleprotocol.xml) which can be used as a reference. This example defines a protocol called "Demolink" which is frequently referenced in this document. Tags and attributes in the xml document are not case sensitive. 

Protocol tag
============

The root element of the XML is "Protocol". It must be present for ProtoGen to generate any output. An example Protocol tag is:

    <Protocol name="Demolink" api="1" version="1.0.0.a" endian="big"
              comment="This is an demonstration protocol definition.">

The Protocol tag supports the following attributes:

- `name` : The name of the protocol. This will set the name of the primary header file for this protocol, and the generic packet utility functions. In this example (and elsewhere in this file) the name is "Demolink". This attribute is mandatory, all other Protcol attributes are optional.

- `file` : Optional attribute that gives the name of the source and header file name (.c and .h) that will be used for all code output except the primary header file, and any objects which have their own `file` attribute.

- `prefix` : A string that can be used to prepend structure and file names. This is typically left out, but if a single project uses multiple ProtoGen protocols then it may be useful to give them different prefixes to avoid namespace collisions.

- `maxSize` : A number that specifies the maximum number of data bytes that a packet can support. If this is provided, and is greater than zero, ProtoGen will issue a warning for any packet whose maximum encoded size is greater than this.

- `api` : An enumeration that can be used to determine API compatibility. Changes to the protocol definition that break backwards compatibility should increment this value. Calling code can access the api value and use it to (for example) seed a packet checksum/CRC to prevent clashes with different versions of the protocol.

- `version` : A human readable version string to describe the protocol. Calling code can access the version string.

- `endian` : By default the generated code will encode to and decode from big endian byte order. Setting this attribute to "little" will cause the generated code to use little endian byte order. This attribute is *not* the byte order of the computer that executes the auto generated code. It *is* the byte order of the data *on the wire*.

- `supportInt64` : if this attribute is set to `false` then integer types greater than 32 bits will not be allowed.

- `supportFloat64` : if this attribute is set to `false` then floating point types greater than 32 bits will not be allowed.

- `supportBitfield` : if this attribute is set to `false` then bitfields will not be allowed.

- `supportLongBitfield` : if this attribute is set to `true` then a second set of bitfield support functions will be defined. The Long bitfield functions use the integer type `uint64_t` instead of `unsigned int`. This attribute will be ignored if `supportInt64` or `supportBitfield` are `false`.

- `bitfieldTest` : if this attribute is set to `true` then the bitfield support module will include a test function that can be used to determine if bitfield support is working on your compiler.

- `supportSpecialFloat` : if this attribute is set to `false` then floating point types less than 32 bits will not be allowed for encoded types.

- `packetStructureSuffix` : This attribute defines the ending of the function names used to encode and decode structures into packets. If not specified then the function name ending is `PacketStructure`. For example the default name of the function that encodes a structure of date information would be `encodeDatePacketStructure()`; using this attribute the name could be changed to (for example) `encodeDatePktStruct()`.

- `packetParameterSuffix` : This attribute defines the ending of the function names used to encode and decode raw parameters into packets. If not specified then the function name ending is `Packet`. For example the default name of the function that encodes a parameters of date information would be `encodeDatePacket()`; using this attribute the name could be changed to (for example) `encodeDatePkt()`.

- `comment` : The comment for the Protocol tag will be placed at the top of the main header file as a multi-line doxygen comment with a \mainpage tag.

Comments
--------

A large part of the value of ProtoGen is the documentation it outputs. To that end every tag supports a comment attribute. Good protocol documentation is verbose, it includes frames of reference, units, the effect on the system of receiving the packet, how often the packet is transmitted, or how to request the packet, etc. In order to make the comment text more readable in the xml file ProtoGen will reflow the comment so that multiple spaces and single linefeeds are discarded. Double linefeeds will be preserved in the output so it is possible to have separate paragraphs in the comment. Some comments are output as multiline doxygen comments wrapped at 80 characters:

    /*!
     * This is a multi-line comment. It appears before the object it documents
     *
     * It supports multiple paragraphs and will be automatically wrapped at 80
     * characters using spaces as separators. It can also contain text that is
     * not reflowed if that text is surrounded with "\verbatim".
     */

Other comments are output as single line doxygen comments:

    //! This is a single line comment that appears before the object it documents.

    //!< This is a single line comment that appears after the object it documents, on the same line.

The reflow logic can be suspended by placing comment text between "\verbatim" escapes. Text between the "\verbatim" escapes is simply output in the comment without reflow or interpretation. This makes it possible to insert more advanced markdown features, such as tables.

Files
-----

By default protogen will output a protocol header file; and a header and source file for every Packet and Structure tag, with the file name matching the tag name. However every global object (Enum, Structure, Packet) supports an optional `file` attribute. In addition there is an optional global `file` attribute which applies if an object does not specify a `file` attribute. Typically an extension is not given, in which case ".h" will be added for header files, and ".c" for source files. If an extension is given then Protogen will discard it unless it starts with ".h" (for headers) or ".c" (for sources). So for example if the `file` attribute of a packet has the extension ".cpp", the source file output by protogen will use that extension, and the header file will use ".h".

The `file` attribute can include path information (for example "src/protogen/filename"). If path information is provided it is assumed to be relative to the global output path, unless the path information is absolute. Path information is only used in the creation of the file; any include directive which references a file will not include the path information.

Require tag
-----------

The Require tag is used to insert XML from an external protocol file at the specified location. The generated code will be structued as if the linked xml was written *at the location it is called* - this tag provides a convenience function for separating the procotol definition into multiple files.

In addition to allowing the protocol to be defined in multiple files, this functionality allows for common features to be inserted into multiple protocol files.

    <Require file="../version.xml"/>
    
The Require tag supports the following attributes:

- `file` : gives the name of the file to insert, relative to the path of the file which requires it. 

The included file must follow the same structure requirements as the base protocol file. However, any top-level attributes specified in the included file (i.e. in the `Protocol` tag) will be ignored.

Include tag
-----------

The Include tag is used to include external C header files in the generated code using a C preprocessor `#include` directive. If the Include tag is a direct child of the Protocol tag then it is emitted in the main header file. If it is a direct child of a Packet or Structure tag then it is emitted in the packet or structures header file. An example Include tag is:

    <Include name="indices.h" comment="indices.h is included for array length enumerations"/>

which produces this output:

    #include "indices.h"   // indices.h is included for array length enumerations

The Include tag supports the following attributes:

- `name` : gives the name of the file to include, with the extension. The name will be wrapped in quotes ("") or angle braces (<>), depending on the `global` attribute, when emitted as part of the #include directive.

- `comment` : Gives a one line comment that follows the #include directive.

- `global` : Set to `true` to use a global header file (e.g., #include \<stdio.h\>) or `false` to use a local include (e.g., #include "indices.h"). The default is `false`.

Enum tag
--------

The Enum tag is used to create a named C language typedef enumeration. The enumeration is typically created in the header file that represents the parent of this tag. An example Enum tag is:

    <Enum name="packetIds" prefix="PKT_" comment="The list of packet identifiers">
        <Value name="ENGINECOMMAND" value="10" comment="Engine command packet"/>
        <Value name="ENGINESETTINGS" comment="Engine settings packet"/>
        <Value name="THROTTLESETTINGS" comment="Throttle settings packet"/>
        <Value name="VERSION" ignorePrefix="true" value="20" comment="Version reporting packet"/>
        <Value name="TELEMETRY" comment="Regular elemetry packet"/>
    </Enum>

which produces this output:

    /*!
     * The list of packet identifiers
     */
    typedef enum
    {
        PKT_ENGINECOMMAND = 10, //!< Engine command packet
        PKT_ENGINESETTINGS,     //!< Engine settings packet
        PKT_THROTTLESETTINGS,   //!< Throttle settings packet
        VERSION = 20,        	//!< Version reporting packet
        PKT_TELEMETRY           //!< Regular elemetry packet
    }packetIds;

Enum tag attributes:
  
- `name` : gives the typedef name of the enumeration

- `file` : Optional attribute that gives the header file name that will be used for this enumeration. The `file` attribute can only be used with enumerations that are global (i.e. not a child of a Packet or Structure tag). If `file` is not provided global enumerations are output in the main header file. Protogen will track the file location of the enumeration and will automatically add the necessary include directive to any module that references the enumeration.

- `comment` : Gives a multi-line line doxygen comment wrapped at 80 characters that appears above the enumeration.

- `prefix` : Gives a string that will be prefixed to the name of each element within the enumeration.

- `description` : If provided, a long-form description can be prepended to the given enumeration (in the documentation markdown). This allows for a more verbose description of the particular enumeration to be added to the docs. NOTE: This description will *not* appear in the generated code.

- `hidden` : is used to specify that this particular enumeration will *not* appear in the generated documentation markdown. NOTE: This enumeration will still appear in the generated code.

- `lookup` : is used to specify that this enumeration allows lookup of label text based on enum values. If enabled, the label for a particular enum value can be returned as a string.f

###Enum : Value subtag attributes:

The Enum tag supports Value subtags; which are used to name individual elements of the enumeration. Attributes of the Value subtag are:

- `name` : gives the name of the enumeration element. This is the name that will be referenced in code.

- `value` : is an optional attribute that sets the value of this enumeration element. If value is left out the element will get its value in the normal C language way (by incrementing from the previous element, or starting at 0). Note that non numeric values may be used, as long as those strings are resolved by an include directive or previous enumeration.

- `comment` : gives a one line doxygen comment that follows the enumeration element.

- `ignorePrefix` : is used to specify that this particular enumeration element will *not* be assigned a prefix (if a prefix is specifed for this enumeration).

- `hidden` : is used to specify that this particular enumeration element will *not* appear in the generated documentation markdown.

In the above example the enumeration support is used to create a list of packet ID values. Although this is the most common use case for this feature, it is not limited to this case. Named enumerations can also be part of the data in a packet. A packet ID enumeration is not required (though it is encouraged as a best practice). Enumerations are also a good choice when creating arrays. If an array length is given by an enumeration that is defined in the protocol xml then ProtoGen will attempt to compute the enumeration value, and use that to compute the length of the array in bytes. This substantially improves the protocol documentation that ProtoGen will output.

Structure tag
-------------
The Structure tag is used to define a structure and the code to encode and decode the structure. Structures can appear under the Protocol tag, in which case they are not associated with any one packet, but can be referenced by any packet.

    <Structure name="Date" comment="Calendar date information">
        <Data name="year" inMemoryType="unsigned16" comment="year of the date"/>
        <Data name="month" inMemoryType="unsigned8" comment="month of the year, from 1 to 12"/>
        <Data name="day" inMemoryType="unsigned8" comment="day of the month, from 1 to 31"/>
    </Structure>

which produces this output:

    /*!
     * Calendar date information
     */
    typedef struct
    {
        uint16_t year;  //!< year of the date
        uint8_t  month; //!< month of the year, from 1 to 12
        uint8_t  day;   //!< day of the month, from 1 to 31
    }Date_t;

Structure tag Attributes:

- `name` : Gives the name of the structure. The structure typename is `prefix + name + _t`. In this case the structure typename is `Date_t`.

- `file` : Gives the name of the source and header file name. If this is ommitted the structure will be written to the name given by the global `file` attribute or, if that is not provided, the `prefix + name` module. If the same file is specified for multiple structures (or packets) then the relevant data are appended to that file.

- `deffile` : Optional attribute used to specify a definition file which receives the structure definition (and any enumeration which is a child of the structure), in place of the normal file location for the structure definition. As with other file attributes the definition file will be correctly appended if it is used multiple times. Any extension to the `deffile` attribute will be ignored.

- `encode` : By default encode functions will be output. This can be suppressed by setting `encode`="false".

- `decode` : By default decode functions will be output. This can be suppressed by setting `decode`="false".

- `comment` : The comment for the structure will be placed at the top of the header file (or the top of the appended text if the file is used more than once).

###Structure : Data subtags

The Structure tag supports Data subtags. Each data tag represents one line in the structure definition. The data tags are explained in more detail in the section on packets.

###Structure : Code subtags

The Structure tag supports Code subtags. Each code tag represents verbatim code that is inserted in either the encode or decode function. In most cases the code tags are not needed, but they can be useful in some cases.

Code subtag attributes:

- `encode` : A verbatim code snippet that is inserted in the encode function.
- `decode` : A verbatim code snippet that is inserted in the decode function.
- `comment` : A one line comment above the code snippet.

Packet tag
----------
The Packet tag is used to define all the code to encode and decode one packet of information. A packet is a superset of a structure. It has all the properties of a structure, and has other capabilities.

    <Packet name="ThrottleSettings" ID="THROTTLESETTINGS"
            comment="Change the throttle control settings. The engine control
                     laws output a throttle command signal from 0 to 1. The
                     data in this packet are used to determine what pulse width
                     to send to the servo for that throttle. The throttle to pulse
                     width mapping can be a straight line, or a linearly interpolated
                     table with up to 10 points.">
  
Packet tag attributes:
  
- `name` : The same as the name attribute of a structure.

- `ID` : gives the identifying value of the packet. This can be any resolvable string, though typically it will be an element of an enumeration. If the ID attribute is missing the all-caps name of the packet is used as the ID. In some cases multiple packets may be identical except for the identifier. It is possible to simply define multiple packets in the XML, however a better solution is to specify multiple identifiers by using spaces or commas to delimit the identifers in the ID attribute. If multiple identifiers are given the encode function is changed so the caller provides the desired id. In addition the decode function is changed so that any of the given identifiers can be used for a successful packet decode.

- `file` : The same as the file attribute of a structure.

- `structureInterface` : If this attribute is set to `true` the structure based interfaces to the packet functions will be created.

- `parameterInterface` : If this attribute is set to `true` then a parameter based interface to the packet functions will be created. This is useful for simpler packets that do not have too many parameters and using a structure as the interface method is unwieldy. If neither `structureInterface` or `parameterInterface` are specified as `true` ProtoGen will output parameter based interfaces if the number of fields in the packet is 1 or less, otherwise it will output structure based interfaces.

- `comment` : The comment for the Packet tag will be placed at the top of the packets header file (or the top of the appended text if the file is used more than once) as a multi-line doxygen comment. The comment will be wrapped at 80 characters using spaces as the separator.

- `hidden` : If set to `true` this attribute specifies that this packet will NOT appear in the documentation markdown.

- `useInOtherPackets` : If set to `true` this attribute specifies that this packet will generate extra outputs as though it were a top level structure in addition to being a packet. This makes it possible to use this packet as a sub-structure of another packet. 

###Packet : Data subtags

The Packet tag supports Data subtags. The Data tag is the most complex part of the packet definition. Each Data tag represents one line in the packet structure definition, and one hunk of data in the packet encoded format. Packets can be created without any Data tags, in which case the packet is empty. Some example Data tags:
	        
    <Data name="numCurvePoints" inMemoryType="bitfield4" comment="Number of points in the throttle curve"/>
    <Data name="reserved" inMemoryType="bitfield3"/>
    <Data name="enableCurve" inMemoryType="bitfield1" comment="Set to enable the throttle curve"/>
    <Structure name="curvePoint" array="10" variableArray="numCurvePoints" comment="table for throttle calibration">
        <Data name="throttle" inMemoryType="float32" encodedType="unsigned8" min="0.0" max="1.0"
              comment="Throttle for this curve point, from 0.0 to 1.0"/>
        <Data name="PWM" inMemoryType="unsigned16"
              comment="PWM in microseconds for this curve point"/>
    </Structure>
    <Data name="lowPWM" inMemoryType="unsigned16"
          comment="PWM in microseconds for 0% throttle"/>
    <Data name="highPWM" inMemoryType="unsigned16"
          comment="PWM in microseconds for 100% throttle"/>

Notice that a structure can be defined inside a packet. Such an implicit structure is declared in the same header file as the packet. Structures can also be members of other structures. This example produces this output:

    /*!
     * table for throttle calibration
     */
    typedef struct
    {
        float    throttle; //!< Throttle for this curve point, from 0.0 to 1.0
        uint16_t PWM;      //!< PWM in microseconds for this curve point
    }curvePoint_t;

    /*!
     * Change the throttle control settings. The engine control laws output a
     * throttle command signal from 0 to 1. The data in this packet are used to
     * determine what pulse width to send to the servo for that throttle. The
     * throttle to pulse width mapping can be a straight line, or a linearly
     * interpolated table with up to 10 points.
     */
    typedef struct
    {
        uint32_t     numCurvePoints : 4; //!< Number of points in the throttle curve
        uint32_t     reserved : 3;
        uint32_t     enableCurve : 1;    //!< Set to enable the throttle curve
        curvePoint_t curvePoint[10];     //!< table for throttle calibration
        uint16_t     lowPWM;             //!< PWM in microseconds for 0% throttle
        uint16_t     highPWM;            //!< PWM in microseconds for 100% throttle
    }ThrottleSettings_t;

Data subtag attributes:

- `name` : The name of the data, not optional (unless `inMemoryType="null"`). This is the name that will be referenced in code.

- `inMemoryType` : The type information for the data in memory, not optional (unless `struct` or `enum` attribute is present). Options for the in-memory type are:
    - `unsignedX` or `uintX_t`: is a unsigned integer with X bits, where X can be 8, 16, 32, or 64.
    - `signedX` or `intX_t` : is a signed integer with X bits, where X can be 8, 16, 32, or 64.
    - `floatX` : is a floating point with X bits, where X can be 32 or 64.
    - `float` : is a 32 bit floating point.
    - `double` : is a 64 bit floating point.
    - `bitfieldX` : is a bitfield with X bits where X can go from 1 to the number of bits in an int, or 64 bits if long bitfields are supported.
    - `string` : is a variable length null terminated string of bytes. The maximum length is given by the attribute `array`.
    - `fixedstring` : is a fixed length null terminated string of bytes. The length is given by the attribute `array`.
    - `null` : indicates empty (i.e. reserved for future expansion) space in the packet.

- `struct` : This attribute replaces the inMemoryType attribute and references a previously defined structure.

- `enum` : This attribute replaces the inMemoryType and references an enumeration. This can be a type name that is defined in an Enum tag above, or is defined by an included file. If the encodedType is not provided then the encoded size of the enumeration is determined automatically by ProtoGen. If encodedType is provided then the size information provided is used to determine the amount of encoded space the enumeration uses. This includes the ability to specify the enumeration encoded as a bitfield.

- `encodedType` : The type information for the encoded data. If this attribute is not provided then the encoded type is the same as the in-memory type. Typically the encoded type will not use more bits than the in-memory type, but this is allowed to support some protocols. If the in-memory type is a bitfield then the encoded type is forced to be a bitfield of the same size. If either the encoded or in-memory type is a string then both types are interpreted as string (or fixed string). If the in-memory type is a `struct` then the encoded type is ignored, since the structure defines its own encding. Options for the encoded type are:
    - `unsignedX` or `uintX_t`: is a unsigned integer with X bits, where X can be 8, 16, 24, 32, 40, 48, 56, or 64.
    - `signedX` or `intX_t` : is a signed integer with X bits, where X can be 8, 16, 24, 32, 40, 48, 56, or 64.
    - `floatX:Y` : is a floating point with X total bits and Y bits of significand, where X can be 16, 24, 32, or 64. See the section on floatspecial for more details
    - `floatX` : is a floating point with X total bits, where X can be 16, 24, 32, or 64.
    - `float` : is a 32 bit floating point.
    - `double` : is a 64 bit floating point.
    - `bitfieldX` : is a bitfield with X bits where X can go from 1 to the number of bits in an int, or 64 bits if long bitfields are supported.
    - `string` : is a variable length null terminated string of bytes. The maximum length is given by the attribute `array`.
    - `fixedstring` : is a fixed length null terminated string of bytes. The length is given by the attribute `array`.
    - `null` : indicates no encoding. The data exist in memory but are not encoded in the packet.

- `bitfieldGroup` : If set to "true" indicates that this bitfield is the first in a group of bitfields. A bitfield group is handled differently then normal bitfields. When decoding a bitfield group the bytes are first decoded from the bytestream, reversing the byte order if the protocol is little endian. Then the bitfields of the group are pulled from the decoded bytes. When encoding a bitfield group the bitfields are put into an array of bytes which are then encoded to the byte stream, reversing the byte order if the protocol is little endian. The `bitfieldGroup="true"` attribute should be applied to the first bitfield of the group, all subsequent bitfields will be assumed to be in the same group. The bitfield group is terminated by a non-bitfield field, or by another bitfield with the `bitfieldGroup="true"` attribute.

- `array` : The array size. If array is not provided then the data are simply one element. `array` can be a number, or an enum, or any defined value from an include file. Note that it is possible to have an array of structures, but not to have an array of bitfields. Although any string that is resolvable at compile time can be used, the generated documentation will be clearer if `array` is a simple number, or an enumeration defined in the protocol; since ProtoGen will be able to calculate the length of the array and the byte location of data that follows it.

- `variableArray` : If this Data are an array, then the `variableArray` attribute indicates that the array length is variable (up to `array` size). The `variableArray` attribute indicates which previously defined data item in the encoding gives the size of the array in the encoding. `variableArray` is a powerful tool for efficiently encoding an a-priori unknown range of data. The `variableArray` variable must exist as a primitive non-array member of the encoding, *before* the definition of the variable array. If the referenced data item does not exist ProtoGen will ignore the `variableArray` attribute. Note: for text it is better to use the encodedType "string", since this will result in a variable length array which is null terminated, and is therefore compatible with C-style string functions.

- `array2d` : For two dimensional arrays, the size of the second dimension. `array2d` is invalid if `array` is not also specified. If `array2d` is not provided the array is one dimensional. It is possible to have an two dimensional array of structures.

- `variable2dArray` : If this Data are a two dimensional array, then the `variable2dArray` attribute indicates that the second dimension length is variable (up to `array2d` size). The `variable2dArray` attribute indicates which previously defined data item in the encoding gives the size of the array in the encoding. The `variable2dArray` variable must exist as a primitive non-array member of the encoding, *before* the definition of the variable array. If the referenced data item does not exist ProtoGen will ignore the `variable2dArray` attribute.

- `dependsOn` : The `dependsOn` attribute indicates that the presence of this Data item is dependent on a previously defined data item. If the previous data item evaluates as zero then this data item is skipped in the encoding. `dependsOn` is useful for encodings that do not know a-priori if a particular data item is available. For example consider a telemetry packet that reports data from all sensors connected to a device: if one of the sensors is not connected or not working then the space in the packet used to report that data can be saved. The `dependsOn` data item will typically be a single bit bitfield, but can be any previous data item which is not a structure or an array. Bitfields cannot be dependent on other data items. ProtoGen will verify that the `dependsOn` variable exists as a primitive non-array member of the encoding, *before* the definition of this data item. If the referenced data item does not exist ProtoGen will ignore the `dependsOn` attribute.

- `min` : The minimum value that can be encoded. Typically encoded types take up less space than in-memory types. This is usually accomplished by scaling the data. `min`, along with `max` and the number of bits of the encoded type, is used to determine the scaling factor. `min` is ignored if the encoded type is floating, signed, or string. If `min` is not given, but `max` is, then `min` is assumed to be 0. `min` can be input as a mathematical expression in infix notation. For example -10000/2^15 would be correctly evaluated as -.30517578125. In addition the special strings "pi" and "e" are allowed, and will be replaced with their correct values. For example 180/pi would be evaluated as 57.295779513082321.

- `max` : The maximum value that can be encoded. `max` is ignored if the encoded type is floating, or string. If the encoded type is signed, then the minimum encoded value is `-max`. If the encoded type is unsigned, then the minimum value is `min` (or 0 if `min` is not given). If `max` or `scaler` are not given then the in memory data are not scaled, but simply cast to the encoded type. `max` can be input as a mathematical expression in the same way as `min`.

- `scaler` : The scaler that is multiplied by the in-memory type to convert to the encoded type. `scaler` is ignored if `max` is present. `scaler` and `max` (along with `min`) are different ways to represent the same thing. For signed encoded types `scaler` is converted to `max` as: `max = ((2^(numbits-1) - 1)/scaler`. For unsigned encoded types `scaler` is converted to `max` as: `max = min + ((2^numbits)-1)/scaler`. `scaler` is ignored if the encoded type is string or structure. If `scaler` or `max` are not given then the in memory data are not scaled, but simply cast to the encoded type. `scaler` can be input as a mathematical expression in the same way as `min`. Although it is unusual `scaler` can be used with floating point encoded types. This would be useful for cases where the units of the floating point encoded type do not match the desired units of the data in memory.

- `default` : The default value for this Data. The default value is used if the received packet length is not long enough to encode all the Data. Defaults can only be used as the last element(s) of a packet. Using defaults it is possible to augment a previously defined packet in a backwards compatible way, by extending the length of the packet and adding new fields with default values so that older packets can still be interpreted. In the case of a `string` or `fixedstring` type the default value is treated as a string of characters rather than converted to a number. In addition, in the string case, if the default string should be empty the default attribute should be set to "null". In addition the special strings "pi" and "e" are allowed, and will be replaced with their correct values; but only if the entire string can be evaluated numerically. Hence "180/pi" will be evaluated as 57.295779513082321 but "piedpiper" will not be altered.

- `constant` : is used to specify that this Data in a packet is always encoded with the same value. This is useful for encodings such as key-length-value which require specific a-priori known values to appear before the data. It can also be useful for transmitting constants such as the API of the protocol. If the encoded type is string then the constant value is interpreted as a string literal (i.e. quotes are placed around it), unless the constant value contains "(" and ")", in which case it is interpreted as a function or macro and no quotes are placed around it. If the inMemoryType is null, the constant value will not be decoded, instead the bytes containing the constant value will simply be skipped over in the decode function. In addition the special strings "pi" and "e" are allowed, and will be replaced with their correct values; but only if the entire string can be evaluated numerically. Hence "180/pi" will be evaluated as 57.295779513082321 but "piedpiper" will not be altered.

- `checkConstant` : If set to "true" this attribute affects the interpretation of `constant` in the decode function. When checkConstant is set the decode function evaluates the decoded data and returns a fail state if the value is not equal to the supplied constant.

- `comment` : A one line doxygen comment that follows the data declaration.

- `range | units | notes` : If specified, each of these attributes will be added (as single-line comments) to the packet description table in the documentation markdown. These comments will appear next to this <Data> tag, and can be used if extra specificity is required. Note that these fields apply ONLY to the documentation, and will NOT appear anywhere in the generated code.

Documentation tag
-----------------

The Documentation tag is used to add extra documentation to the markdown output, but not the generated code. It is typically used to provide expository text needed to better document the overall protocol. The tag can appear anywhere in the xml, at the global level (same as Packet tags), or as a sub-tag of Packet or Enum. For global Documentation tags the order of the tag (with respect to global Packet and Enum tags) is preserved in the markdown output; in combination with the heading label and outline level this makes it possible to better organize the markdown output.

Documentation attributes:

- `name` : The heading of the documentation. This attribute is optional, if it is not given then no heading label is output.

- `comment` : The expository text for this documentation. The text will be reflowed and wrapped at 80 characters. Multiple paragraphs are supported by using two line feeds (a la markdown) to separate the paragraphs.

- `paragraph` : The outline level applied to the heading. This attribute is optional, if it is not given the outline level is assumed to be 1 for global Docmentation and 3 for Documentation that is a sub of Packet or Enum. The `paragraph` attribute only effects the output if the `name` attribute is given.

- `file` : A file that provides the text of the Documentation. This attribute is optional, its purpose is to make it possible to have large blocks of text, with markdown formatting, without having to incude the text directly in the .xml file. The file path is relative to the xml input file path. The contets of the file are inserted into the markdown output without interpretation.

Documentation examples:

    <Documentation comment="---"/>

Inserts "---" into the markdown output. This will produce a horizontal rule in the generated html.

    <Documentation name="Enumerations" paragraph="1" comment="Packets in the protocol refer to these global enumerations."/>

Is an example of documentation that would be used ahead of a global enumeration for packet identifiers. This will insert a level 1 heading named "Enumerations" followed by a paragraph of the text given in comment.

---

The generated top level code
============================

ProtoGen will create a header file with the same name as the protocol. This module defines any enumeration that is a direct child of the Protocol tag. Most commonly this will be an enumerated list of packet identifiers. It will include any files that the protocol xml indicates (that are not a sub of a packet definition). Since the generated protocol header file is included in every structure and packet file this implies that the files included here will also be part of every other header. The top level module will also provide functions to retrieve information about the protocol. These functions are only included if the protocol ICD includes `API` and/or `Version` attributes:

    //! Return the protocol API enumeration
    int getDemolinkApi(void);

    //! Return the protocol version string
    const char* getDemolinkVersion(void);

In the example above the string "Demolink" is the name of the protocol and comes from the protocol xml.

Generic Packets
---------------

Despite the name ProtoGen is not really a protocol or packet generator. It is instead an encode/decode generator. By design ProtoGen does not know (or care) about specific packet details. The low level packet routines do not often change and therefore do not realize the same auto-coding advantages as the encoding/decoding code. ProtoGen assumes the following about packets: Packets transport a hunk of data which is internally represented as an array of bytes. Packets have an identifier number that designates their function. Packets have a size, which describes the number of bytes of data they transport. These assumptions are represented by five functions whose prototypes are created by ProtoGen and which the generated code calls (you provide the implementation):

    //! Return the packet data pointer from the packet
    uint8_t* getDemolinkPacketData(void* pkt);

    //! Return the packet data pointer from the packet
    const uint8_t* getDemolinkPacketDataConst(const void* pkt);

    //! Complete a packet after the data have been encoded
    void finishDemolinkPacket(void* pkt, int size, uint32_t packetID);

    //! Return the size of a packet from the packet header
    int getDemolinkPacketSize(const void* pkt);

    //! Return the ID of a packet from the packet header
    uint32_t getDemolinkPacketID(const void* pkt);

Each function takes a pointer to a packet. The implementation of the function will cast this void pointer to the correct packet structure type. When the generated encode routine encodes data into a packet it will first call `getDemolinkPacketData()` to retrieve a pointer to the start of the packets data buffer. When the generated code finishes the data encoding it will call `finishProtocolPacket()`, which can then perform any work to complete the packet (such as filling out the header and or generating the CRC). In the reverse direction the decode routine checks a packets ID by comparing the return of `getDemolinkPacketID()` with the ID indicated by the protocol ICD. The decode routine will also verify the packet size meets the minimum requirements.

Some packet interfaces use multiple ID values. An example would be the uBlox GPS protocol, each packet of which includes a "group" as well as an "ID". For this reason ProtoGen gives 32-bits for the ID value. The intent is that multiple IDs (which are typically less than 32-bits) can be concatenated into a single value for ProtoGens purposes.  

The generated packet code
=========================

The packet structure
--------------------

Each packet defined in the protocol xml will produce code that defines a structure to represent the data-in-memory that the packet transports. In some implementations you may write interface glue code to copy the actual in memory data to this structure before passing it to the generated routines. In other implementations the structures defined by ProtoGen will be used directly in the rest of the project. This is the intended use case and is the purpose of defining separate "in memory" and "encoded" data types. You can use whatever in memory data type best fits your use case without worrying (much) about the impact on the protocol efficiency. The structure defined by a packets generated code will have a typedef name like "prefixPacket_t" where "prefix" is the protocol prefix (if any) given in the xml and "Packet" is the name of the packet from the xml.

    /*!
     * Version information for the software and hardware. Send this packet with zero
     * length to request the version information.
     */
    typedef struct
    {
        Board_t board;           //!< details about this boards provenance
        uint8_t major;           //!< major version of the software
        uint8_t minor;           //!< minor version of the software
        uint8_t sub;             //!< sub version of the software
        uint8_t patch;           //!< patch version of the software
        Date_t  date;            //!< the release date of the software
        char    description[64]; //!< description of the software version
    }Version_t;

The packet length and ID
------------------------

The autogenerated code will define any named enumeration given in the protocol xml for the packet. It will also define macros to determine the packet ID and the minimum length of the encoded data of the packet. The packet lengths are given in numbers of bytes. ProtoGen refers to "minimum length", rather than "length" because packets can include default data, variable length arrays, or strings, any of which can cause the packet length to vary.

    //! return the packet ID for the Version packet
    #define getVersionPacketID() (VERSION)

    //! return the minimum data length for the Version packet
    #define getVersionMinDataLength() (25)


Variable length packets are a great tool for optimizing bandwidth utilization without limiting the in-memory capabilities of the code. However the correct length of a variable length packet cannot be determined until the packet has been mostly decoded; therefore the generated code will check the length of such a packet twice. The first check occurs before any decoding is done to verify the packet meets the minimum length. The second check occurs when the packet decoding is complete (or only default fields are left) to verify that there were enough bytes to complete the variable length fields.

Packet encoding and decoding functions
------------------------------------------

The autogenerated code will define functions to encode and decode the packets. These functions convert the in-memory representation to and from the encoded representation. These are the functions that your code will most likely interface with. Notice that there are two forms to the packet functions. The form that ends in "Structure" takes a pointer to a structure (`Version_t` in this example) defined by this packets module. The other form allows you to specify individual data fields as parameters. Which form you use is up to you based on how you want to keep the data in memory. Which form is output in the generated code is a function of the attributes `parameterInterface` and `structureInterface`.

    //! Create the Version packet
    void encodeVersionPacketStructure(void* pkt, const Version_t* user);

    //! Decode the Version packet
    int decodeVersionPacketStructure(const void* pkt, Version_t* user);

    //! Create the Version packet
    void encodeVersionPacket(void* pkt, const Board_t* board, uint8_t major, uint8_t minor,
                 uint8_t sub, uint8_t patch, const Date_t* date, const char description[64]);

    //! Decode the Version packet
    int decodeVersionPacket(const void* pkt, Board_t* board, uint8_t* major, uint8_t* minor,
                 uint8_t* sub, uint8_t* patch, Date_t* date, char description[64]);

The packet is allocated by the caller and passed in via void pointer. Using void pointer means that the generated code does not need to know the low level packet details. See the section on Generic Packets for the means by which the generated code will interface to the packet definition. Notice that the decode function returns `int`. If you attempt to decode a packet whose ID does not match the xml description, or whose size is not sufficient, then `0` will be returned. In that case the `user` structure will be unchanged if the packet is a fixed length; but if the packet is variable length the structure may have been modified with bogus data. Hence the return value from the decode function *must* be checked.

Structure encoding and decoding functions
---------------------------------------------

In addition to the packet functions the generated code can include functions to encode and decode a structure to a byte array. These functions can be used if you do not want the generated code to interact with packet routines. Perhaps because your data are not being encoded in packets, or the simple packet interfaces that ProtoGen supports are not sophisticated enough. These functions will only be generated for Structure tags, not Packet tags. If you want to simultaneously have a structure with generic encoding functions and packet functions; then create both a structure tag and a packet tag which references the structure (see the GPS packet in exampleprotocol.xml).

	//! Create the GPS packet
    void encodeGPSPacket(void* pkt, const GPS_t* gps);

    //! Decode the GPS packet
    int decodeGPSPacket(const void* pkt, GPS_t* gps);

Note that Structures do not emit markdown documentation. If a structure is included as part of a packet, then its documentation will be output when the packets documentation is output, in order to complemetely document the packet.

Parsing order
-------------

Global tags (those directly under the `Protocol` tag) are not parsed in the order defined in the xml. Instead ProtoGen will first parse global Enums, and then global Structures, and finally global Packets. Enums and Structures which are subs of global tags are parsed when their parent tag is parsed. This parsing order is important because Structures may refer to Enums, and Packets may refer to both Enums and Structures. The parsing order ensures that the references are available when needed.

However the order of the global tags (in the xml) is preserved when the markdown documentation is output. This makes it possible to organize the xml file to produce the best documentation.

Other generated code
====================

ProtoGen also creates other files that are not specified by the xml, but are used as helper functions for the generated packet code. These are the modules: bitfieldspecial, floatspecial, fieldencode, fielddecode, scaledencode, scaleddecode. Although these modules are not specified by the xml they are still generated. Much of the code in these modules is tedious and repetitive, so it was ultimatley simpler and less error prone to auto generate it. More importantly automatically generating this code makes it easier for future versions of ProtoGen to take advantage of changes or advances in the routines these modules provide.

fieldencode and fielddecode
---------------------------

fieldencode does the work of moving data from a native type to an array of bytes. This is not as simple as you might think. In the simplest case one could simply copy the data from one memory location to another (for example, using `memcpy()`); however that would ignore the complications caused by local byte order (big or little endian) and memory alignment requirements. For example, if the processor is little endian (e.g. x86 or ARM) but the protocol is big endian the bytes of the native type have to be swapped. Furthermore, we do not necessarily know the alignment of the packet data pointer, so copying a four byte mis-aligned native type (for example) may not be allowed by the processor.

The way to handle both these issues is to copy the data byte by byte. There are numerous methods by which this can be done. ProtoGen does it using leftshift (`<<`) and rightshift (`>>`) operators. This has the advantage of (potentially) leaving the native type in a register during the copy, and not needing to know the local endianness. Even this operation has room for interpretation. The maximum number of bits that can be shifted is architecture dependent; but is typically the number of bits of an `int`. Hence the process of shifting the bits from the field to the data array (and vice versa) is ordered such that only 8 bit shifts are used, allowing ProtoGen code to run on 8 bit processors.

fieldencode also provides the routines to encode non native types, such as `int24_t`. `int24_t` is a 24 bit signed type, which does not exist in most computer architectures. Instead fieldencode provides routines to take a `int32_t` and encode it as a `int24_t`, by discarding the most significant byte. Routines are provided for every byte width from 1 byte to 8 bytes, for both signed and unsigned numbers. If you set the protocol attribute `supportInt64="false"` then support for integer types greater than 32 bits will be omitted. This removes a lot of functions from this module. Note that you can still encode double precision floating points in this case. To disable double precision floating points then set the protocol attribute `supportFloat64="false"`.

fielddecode provides the decoding routines that are the corollary to the routines in fieldencode. These are slightly more challenging for non-native signed types, because special code must be added to perform sign extension of such types when they are converted to the next largest native type.

scaledencode and scaleddecode
-----------------------------

scaledencode and scaleddecode provide routines to take an in-memory number (like a `double` for example) and scale it to any smaller size integer. An example use case would be to take a double precision geodetic latitude in radians and encode it as a 32-bit signed integer number. Doing so would provide better than centimeter resolution for the encoded type, while using only half the space in the encoded format as the in-memory format. Routines are not provided to inflate the size of an in-memory type. For example there is no routine to take a float and encode it in a 64-bit integer. This would simply waste datalink bandwidth without providing any resolution improvement.

If you set the protocol attribute `supportInt64="false"` then support for integer types greater than 32 bits will be omitted. This removes a *lot* of functions from this module. Note that you can still encode scaled double precision floating points in this case (as long as you scale them to 32 bits or less). To disable double precision floating points then set the protocol attribute `supportFloat64="false"`.

floatspecial
------------

floatspecial provides routines to convert to and from normal floating point types (64 and 32 bit) to the special types also supported by ProtoGen (namely, 16 and 24 bits with a variable significand size). One could legitimately ask why such types are needed: isn't it better to simply scale a 32 (or 64) bit float down to a smaller integer? Usually yes, but there are cases where this is not ideal. A good example is the amount of fuel on board an aircraft: Typically the accuracy of the fuel level estimate or sensor is no more than 8 bits. However the dynamic range of the fuel level is a function of the aircraft type. A 4 pound drone might carry 1 pound of fuel. An A380 airliner might carry 500,000 lbs of fuel. There is no simple way to scale the in memory floating point fuel level to a smaller integer type without a-priori knowing the size of vehicle (or using an excessive number of bits). However a 16-bit floating point type with 9 bits of resolution can do this nicely. So for that case, supporting a 16-bit float allows us to save 2 bytes in the encoded data stream.

The 16 and 24 bit float formats use the same layout as IEEE-754: the most signficant bit is a sign bit, the next bits are the biased exponent, and the least significant bits are the significand with an implied leading 1. By default the 16 bit float uses 6 bits of expononent with 9 bits of significand, and the 24-bit format uses the same number of exponent bits as float32 (8 bits of exponent). Hence the default float24 covers the same range as a float32, but with 15 bits of resolution rather than 23 bits. The default 16-bit format uses 6 bits for exponent, so it has a range that is 1/4 of float32 (aproximately -2^31 to 2^31), but with 9 bits of resolution rather than 23 bits. The 9 and 15 bit signficand floats are what you get if the encoded type is "float16" or "float24" respectively. However you can specify the float in more detail by adding another number. Asking for a "float16:10" encoded type will result in a 16-bit float with 5 bits of exponent and 10 bits of significand.

floatspecial also provides routines to determine if a pattern of 32 or 64 bits is a valid `float` or `double`. In the case where a native floating point type is decoded directly from the byte stream (as opposed to being scaled from integer) these functions are used to make sure the floating point number is not infinity, NaN, or denormalized prior to loading the value into a floating point register. This is important for many embedded processors which have limited floating point environments that will throw an exception in the event of an invalid floating point. Any invalid floating point that is decoded is replaced with 0.

ProtoGen assumes that the `float` (32-bit) and `double` (64-bit) types adhere to IEEE-754. ProtoGen's assumption of the layout of the `float` and `double` types is only a factor in two cases: 1) if the protocol you specify uses 16 or 24 bit floating point types (i.e. if a conversion between the types is needed) and 2) if a native 32 or 64 bit float type is encoded without scaling by integer, which will trigger the check to determine if the float is valid when it is decoded. If any of your processors do not adhere to the IEEE-754 spec for floating point, do not use 16 or 24 bit floats in your protocol ICD. If you set the protocol attribute `supportSpecialFloat="false"` then the floatspecial module will not be emitted and any reference to float16 or float24 in the protocol will generate a warning and the type will be changed to float32. In addition setting `supportSpecialFloat="false"` will cause ProtoGen to skip the valid float check on decode.

bitfieldspecial
---------------

bitfieldspecial provides routines for encoding and decoding bitfields into and out of byte arrays. If you set the protocol attribute `supportBitfield="false"` this file will not be output. In addition any bitfields in the protocol description will generate a warning, and the field will be converted to the next larger in-memory unsigned integer. If you use a bitfield which is larger than 32 bits, and if `supportLongBitfield` is set to `"true"` the bitfield type will be uint64_t, and the long bitfield functions will be used for that bitfield. The normal bitfield support routines use `unsigned int` as the base type; this has the advantage of working on all compilers. If the protocol attribute `bitfieldTest="true"` a test function will be written into bitfieldspecial which can be used to test the bitfield routines on your compiler.

Bitfields are fantastically useful for minimizing the size of packets, however there is some ambiguity when it comes to byte ordering within a bitfield. Since the byte boundaries are not fixed at 8-bit intervals a bitfield cannot be described as big or little endian. ProtoGen encodes bitfields with the most significant bits first in the data stream, and the least significant bits last. This can be changed by putting the bitfields into a "bitfield group", see the section on bitfield groups for more details.

Although bitfields are typically used to convey integer or enumeration information, it is possible to scale an in-memory type to a bitfield. The only drawback to this usage case is that ProtoGen will not enforce the minimum or maximum value that can be encoded. The normal scaling routines in scaleencode will check for and handle an overflow or underflow, because those routines know at compile time how many bits are available. Since the bitfield scaling routines do not know at compile time how many bits are being used, adding checks for overflow or underflow would be too burdensome. User beware; if you choose to scale a value to a bitfield you must ensure that the scaled value does not overflow the encoded bits.

Generation of documentation
===========================

Documentation for a protocol is often the last task undertaken by a developer, but arguably should be the first. The usefulness of a protocol is largely dictated by the quality of its documentation (unless you plan to be the only one to ever use it!). ProtoGen provides automatic documentation. The emitted code will be decorated with doxygen style comments. If doxygen is installed ProtoGen can use it to generate html documentation for the code. This documentation is useful for developers who are coding against the API provided by the generated code.

ProtoGen will also output a markdown file which provides a MultiMarkdown formatted document of the protocol. ProtoGen can feed this file to MultiMarkdown (if it is installed) in order to create a html file. This file is intended as a master document for the protocol that is suitable for developers and users alike. The author of the protocol xml is encouraged to be verbose in the comment attributes. The better you document your protocol the less users will bug you for help!

###Documentation consequences of using the `Include` tag

The `Include` tag is most useful when integrating ProtoGen code with pre-existing software. However using it reduces the usefulness of ProtoGen's documentation. ProtoGen is not a compiler; it will not read the referenced include file and resolve all strings contained therein. For example if you define an array length enumeration (e.g. "#define N3D 3") in an included file and then use that as an array attribute (`array="N3D"`) ProtoGen will not *know* that the array is 3 elements long. The generated code will be perfectly readable and correct; however the ambiguous array length means that ProtoGen will not be able to document the exact byte location of the data in the packet.

A similar situation occurs if you use an external file to specify packet IDs, or enumerations. Externally defined IDs mean that ProtoGen cannot document the exact ID value, only the string that is given in the protocol xml. The documentation is therefore incomplete, and any user of the protocol will need to refer to the externally included file.

Note that the reverse situation does not occur. If you use ProtoGen to define enumerations and IDs then other code can easily utilize them by including the header file(s) output by ProtoGen. This is the best way to integrate ProtoGen with external code as it results in better documentation.

---

About the author
===========================

Contact information:

Bill Vaglienti

Five by Five Development, LLC

[www.fivebyfivedevelopment.com](http://www.fivebyfivedevelopment.com)
 
Before founding Five by Five Development in 2013 I was co-founder of [Cloud Cap Technology](http://www.cloudcaptech.com), which developed the Piccolo flight management system. Although Piccolo was at heart a flight controller, I spent more time coding communications protocols than anything else. Hopefully ProtoGen will allow myself and others to spend less time doing that.
