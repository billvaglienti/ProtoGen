Introduction
============

ProtoGen is short for "protocol generation". ProtoGen owes its provenance to an often repeated problem: how to efficiently and in a cross platform way communicate information from one computer to another. This problem appears in many places, but most commonly in connecting microcontroller-based peripheral devices (GPSs, Autopilots, IMUs, etc) together. These devices maintain a sophisticated internal state representation in memory; and communicating this data efficiently and in a cross platform way is non-obvious. By efficient I mean: efficient in the amount of bandwidth used. By cross platform I mean: different computer architectures, memory order, compilers, and register bit widths should not cause problems for the communication.

The naive way of doing this is to copy the data in memory into a packet which is sent to the other computer. As an example consider a GPS: it outputs a three dimensional position estimate. Internally the position is represented as three double precision numbers (latitude, longitude, altitude, or perhaps ECEF x, y, z), occupying a total of 24 bytes of memory. When using LLA or ECEF the position number needs enough dynamic range to represent the position to a resolution which is better than the GPS accuracy. As an example if we want ECEF position resolution of 0.01 meters, then we need approximately log2(7,000,000/0.01) ~ 30 bits. This is why double precision is required, since single precision floating point only gives 23 bits of resolution.

If one were to simply `memcpy()` the double precision representation into a packet there are a number of negative consequences:

1. The number can fit in a dynamic range of 32-bits, but 64 bits are used; doubling the amount of bandwidth required.
2. The receiver may use a different byte order then the sender (little versus big endian).
3. The receiver may use different structure packing rules then the sender.
4. The receiver may have different memory alignment rules then the sender.
5. The receiver's definition of floating point numbers may be different from the sender. (To be fair, most systems now use [IEEE-754 for floating point](http://grouper.ieee.org/groups/754/), so this is unlikely).

These problems can be averted if the internal data representation is converted to a common representation as given by an interface control document (ICD) before transmission. The receiver then un-converts the received data according to the same ICD. The conversion can account for all of the problems that can occur when using simple copying. Most peripheral devices use some version of this idea: the device comes with an ICD that defines how to interpret the transmitted data. Software is then written according to this ICD to implement the encoding/decoding rules. However this creates new problems:

1. Implementing communications now requires a lot more software development. You cannot just `memcpy()` to or from the packet data buffer.
2. CPU time is spent at both the receiver and transmitter to perform the ICD interpretation.
3. The communications protocol is typically written at least three times: the encode (transmit), the decode (receive), and the documentation (ICD). Keeping all three implementations in sync is challenging and a common source of bugs.

ProtoGen is a tool that takes a xml protocol description and generates html for documentation, and C source code for encoding and decoding the data. This alleviates much of the challenge and bugs in protocol development. The C source code is highly portable, readable, efficient, and well commented. It is suitable for inclusion in almost any C/C++ compiler environment.

This document refers to ProtoGen version 1.1.0. You can download the [windows version here](http://www.fivebyfivedevelopment.com/Downloads/ProtoGenWin.zip). The [mac version is here](http://www.fivebyfivedevelopment.com/Downloads/ProtoGenMac.zip).

---

Usage
=====

ProtoGen is a C++/Qt5 compiled command line application, suitable for inclusion as a automated build step (Qt provides the xml, string, and file handling). The command line is: `ProtoGen Protocol.xml [Outputpath] [-no-doxygen] [-no-markdown] [-no-helper-files]`. `Protocol.xml` is the file that defines the protocol details. `Outputpath` is an optional parameter that gives the path where the generated files should be placed. If `Outputpath` is not given then the files will be placed in the working directory from which ProtoGen is run. `-no-doxygen` will cause ProtoGen to skip the output of the developer level html documentation. `-no-markdown` will cause ProtoGen to skip the output of the user level html documentation. `-no-helper-files` will cause ProtoGen to skip the output of files not directly specified by the protocol.xml.

Dependencies
------------

ProtoGen contains the [Qt dependencies](http://qt-project.org/) in its distribution, so Qt does not need to be installed on the host system. ProtoGen relies on [Markdown](http://daringfireball.net/projects/markdown/) for its user level html documentation. The Markdown script is embedded in ProtoGen, but the host system must have perl installed. In addition the code generated by ProtoGen contains doxygen markup comments. If the host system has [doxygen](http://www.stack.nl/~dimitri/doxygen/index.html) installed ProtoGen can output developer level html documentation. On windows doxygen is invoked by ProtoGen as simply "doxygen" implying that doxygen.exe is part of the path. On mac doxygen is invoked by ProtoGen as "/Applications/Doxygen.app/Contents/Resources/doxygen"; this is because doxygen for the mac is provided as a .app bundle containing both the GUI and the binary.

Protocol ICD
================

The protocol is defined in an [xml](http://www.w3.org/XML/) document. ProtoGen comes with an example document (exampleprotocol.xml) which can be used as a reference. This example defines a protocol called "Demolink" which is frequently referenced in this document.

Protocol tag
============

The root element of the XML is "Protocol". It must be present for ProtoGen to generate any output. An example Protocol tag is:

    <Protocol name="Demolink" api="1" version="1.0.0.a" endian="big"
              comment="This is an demonstration protocol definition.">

The Protocol tag supports the following attributes:

- `name` : The name of the protocol. This will set the name of the primary header file for this protocol, and the generic packet utility functions. In this example (and elsewhere in this file) the name is "Demolink".

- `prefix` : A string that can be used to prepend structure and file names. This is typically left out, but if a single project uses multiple ProtoGen protocols then it may be useful to give them different prefixes to avoid namespace collisions. A common way to use the `prefix` is to make it the same as `name`.

- `api` : An enumeration that can be used to determine API compatibility. Changes to the protocol definition that break backwards compatibility should increment this value. Calling code can access the api value and use it to (for example) seed a packet checksum/CRC to prevent clashes with different versions of the protocol.

- `version` : A human readable version string to describe the protocol. Calling code can access the version string.

- `endian` : By default the generated code will encode to and decode from big endian byte order. setting this attribute to "little" will cause the generated code to use little endian byte order. This attribute is *not* the byte order of the computer that executes the auto generated code. It *is* the byte order of the data *on the wire*.

- `supportInt64` : if this attribute is set to `false` then integer types greater than 32 bits will not be allowed.

- `supportFloat64` : if this attribute is set to `false` then floating point types greater than 32 bits will not be allowed.

- `supportBitfield` : if this attribute is set to `false` then bitfields will not be allowed.

- `supportSpecialFloat` : if this attribute is set to `false` then floating point types less than 32 bits will not be allowed for encoded types.

- `comment` : The comment for the Protocol tag will be placed at the top of the main header file as a multi-line doxygen comment with a \mainpage tag.

Comments
--------

A large part of the value of ProtoGen is the documentation it outputs. To that end every tag supports a comment attribute. Good protocol documentation is verbose, it includes frames of reference, units, the effect on the system of receiving the packet, how often the packet is transmitted, or how to request the packet, etc. In order to make the comment text more readable in the xml file ProtoGen will reflow the comment so that multiple spaces and single linefeeds are discarded. Double linefeeds will be preserved in the output so it is possible to have separate paragraphs in the comment. Some comments are output as multiline doxygen comments wrapped at 80 characters:

    /*!
     * This is a multi-line comment. It appears before the object it documents
     *
     * It supports multiple paragraphs and will be automatically wrapped at 80
     * characters using spaces as separators.
     */

Other comments are output as single line doxygen comments:

    //! This is a single line comment that appears before the object it documents.

    //!< This is a single line comment that appears after the object it documents, on the same line.

Include tag
-----------

The Include tag is used to include other files in the generated code by emmitting a C preprocessor `#include` directive. If the Include tag is a direct child of the Protocol tag then it is emitted in the main header file. If it is a direct child of a Packet or Structure tag then it is emitted in the packet or structures header file. An example Include tag is:

    <Include name="indices.h" comment="indices.h is included for array length enumerations"/>

which produces this output:

    #include "indices.h"   // indices.h is included for array length enumerations

The Include tag supports the following attributes:

- `name` : gives the name of the file to include, with the extension. The name will be wrapped in quotes ("") when emitted as part of the #include directive.

- `comment` : Gives a one line comment that follows the #include directive.

Enum tag
--------

The Enum tag is used to create a named C language typedef enumeration. As with the Include tag the enumeration is created in the header file that represents the parent of this tag. An example Enum tag is:

    <Enum name="packetIds" comment="The list of packet identifiers">

Enum tag attributes:
  
- `name` : gives the typedef name of the enumeration

- `comment` : Gives a multi-line line doxygen comment wrapped at 80 characters that appears above the enumeration.

###Enum : Value subtag attributes:

The Enum tag supports Value subtags; which are used to name individual elements of the enumeration. An example is:

    <Enum name="packetIds" comment="The list of packet identifiers">
        <Value name="ENGINECOMMAND" value="10" comment="Engine command packet"/>
        <Value name="ENGINESETTINGS" comment="Engine settings packet"/>
        <Value name="THROTTLESETTINGS" comment="Throttle settings packet"/>
        <Value name="VERSION" value="20" comment="Version reporting packet"/>
        <Value name="TELEMETRY" comment="Regular elemetry packet"/>
    </Enum>

which produces this output:

    /*!
     * The list of packet identifiers
     */
    typedef enum
    {
        ENGINECOMMAND = 10,  //!< Engine command packet
        ENGINESETTINGS,      //!< Engine settings packet
        THROTTLESETTINGS,    //!< Throttle settings packet
        VERSION = 20,        //!< Version reporting packet
        TELEMETRY            //!< Regular elemetry packet
    }packetIds;

- `name` : gives the name of the enumeration element. This is the name that will be referenced in code.

- `value` : is an optional attribute that sets the value of this enumeration element. If value is left out the element will get its value in the normal C language way (by incrementing from the previous element, or starting at 0). Note that non numeric values may be used, as long as those strings are resolved by an include directive or previous enumeration.

- `comment` : gives a one line doxygen comment that follows the enumeration element.

In the above example the enumeration support is used to create a list of packet ID values. Although this is the most common use case for this feature, it is not limited to this case. Named enumerations can also be part of the data in a packet. A packet ID enumeration is not required (though it is encouraged as a best practice).

Structure tag
-------------
The Structure tag is used to define a structure and the code to encode and decode the structure. Structures can appear under the Protocol tag, in which case they are not associated with any one packet, but can be referenced by any following packet.

    <Structure name="Date" file="Calendar" comment="Calendar date information">
    
Structure tag Attributes:

- `name` : Gives the name of the structure. The structure typename is the `prefix + name + _t`. In this case the structure typename is `Date_t`.

- `file` : Gives the name of the source and header file name (.c and .h). If this is ommitted the structure will be written to the `prefix + name` module. If the same file is specified for multiple structures (or packets) then the relevant data are appended to that file.

- `comment` : The comment for the structure will be placed at the top of the header file (or the top of the appended text if the file is used more than once.) 

###Structure : Data subtags

The Structure tag supports Data subtags. Each data tag represents one line in the structure definition. In the Date structure example the Data tags are simple. The data tags are explained in more detail in the section on packets.
    
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

- `ID` : gives the identifying value of the packet. This can be any resolvable string, though typically it will be an element of a previously defined enumeration. If the ID attribute is missing then the all-caps name of the packet is used as the ID. 

- `file` : The same as the file attribute of a structure.

- `structureInterface` : If this attribute is set to `true` the structure based interfaces to the packet functions will be created.

- `parameterInterface` : If this attribute is set to `true` then a parameter based interface to the packet functions will be created. This is useful for simpler packets that do not have too many parameters and using a structure as the interface method is unwieldy. If neither `structureInterface` or `parameterInterface` are specified as `true` ProtoGen will output parameter based interfaces if the number of fields in the packet is 1 or less, otherwise it will output structure based interfaces.

- `comment` : The comment for the Packet tag will be placed at the top of the packets header file as a multi-line doxygen comment. The comment will be wrapped at 80 characters using spaces as the separator.

###Packet : Data subtags

The Packet tag supports data subtags. The data tag is the most complex part of the packet definition. Each Data tag represents one line in the packets structure definition, and one hunk of data in the packets encoded format. Packets can be created without any Data tags, in which case the packet is empty. Some example Data tags:
	        
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

- `name` : The name of the data, not optional. This is the name that will be referenced in code.

- `inMemoryType` : The type information for the data in memory, not optional (unless `struct` or `enum` attribute is present). Options for the in-memory type are:
    - `unsignedX` or `uintX_t`: is a unsigned integer with X bits, where X can be 8, 16, 32, or 64.
    - `signedX` or `intX_t` : is a signed integer with X bits, where X can be 8, 16, 32, or 64.
    - `floatX` : is a floating point with X bits, where X can be 32 or 64.
    - `float` : is a 32 bit floating point.
    - `double` : is a 64 bit floating point.
    - `bitfieldX_t` : is a bitfield with X bits where X can go from 1 to 32 bits.
    - `string` : is a variable length null terminated string of bytes. The maximum length is given by `array`.
    - `fixedstring` : is a fixed length null terminated string of bytes. The length is given by `array`.
    - `null` : indicates empty (i.e. reserved for future expansion) space in the packet.

- `struct` : This attribute replaces the inMemoryType attribute and references a previously defined structure.

- `enum` : This attribute replaces the inMemoryType and references an enumeration. This can be a type name that is defined in an Enum tag above, or is defined by an included file.

- `encodedType` : The type information for the encoded data. If this attribute is not provided then the encoded type is the same as the in-memory type. The encoded type cannot use more bits than the in-memory type. If the in-memory type is a bitfield then the encoded type is forced to be a bitfield of the same size. If either the encoded or in-memory type is a string then both types are interpreted as string (or fixed string). If the in-memory type is a `struct` then the encoded type is ignored. Options for the encoded type are: 
    - `unsignedX` or `uintX_t`: is a unsigned integer with X bits, where X can be 8, 16, 24, 32, 40, 48, 56, or 64.
    - `signedX` or `intX_t` : is a signed integer with X bits, where X can be 8, 16, 24, 32, 40, 48, 56, or 64.
    - `floatX` : is a floating point with X bits, where X can be 16, 24, 32, or 64.
    - `float` : is a 32 bit floating point.
    - `double` : is a 64 bit floating point.
    - `bitfieldX_t` : is a bitfield with X bits where X can go from 1 to 32 bits.
    - `string` : is a variable length null terminated string of bytes. The maximum length is given by `array`.
    - `fixedstring` : is a fixed length null terminated string of bytes. The length is given by `array`.
    - `null` : indicates no encoding. The data exist in memory but are not encoded in the packet.

- `array` : The array size. If array is not provided then the data are simply one element. `array` can be a number, or an enum, or any defined value from an include file. Note that it is possible to have an array of structures, but not to have an array of bitfields.

- `variableArray` : If this data are an array, then the `variableArray` attribute indicates that the array length is variable (up to `array` size). The `variableArray` string indicates which previously defined data item in the encoding gives the size of the array in the encoding. `variableArray` is a powerful tool for efficiently encoding an a-priori unknown range of data. The `variableArray` variable must exist as a primitive non-array member of the encoding, *before* the definition of the variable array. If the referenced data item does not exist ProtoGen will ignore the `variableArray` attribute. Note: for text it is better to use the encodedType "string", since this will result in a variable length array which is null terminated, and is therefore compatible with C style string functions.

- `dependsOn` : The `dependsOn` string indicates that the presence of this Data item is dependent on a previously defined data item. If the previous data item evaluates as zero then this data item is skipped in the encoding. `dependsOn` is useful for encodings that do not know a-priori if a particular data item is available. For example consider a telemetry packet that reports data from all sensors connected to a device: if one of the sensors is not connected or not working then the space in the packet used to report that data can be saved. The `dependsOn` data item will typically be a single bit bitfield, but can be any previous data item which is not a structure or an array. Bitfields cannot be dependent on other data items. ProtoGen will verify that the `dependsOn` variable exists as a primitive non-array member of the encoding, *before* the definition of this data item. If the referenced data item does not exist ProtoGen will ignore the `dependsOn` attribute.

- `min` : The minimum value that can be encoded. Typically encoded types take up less space than in-memory types. This is usually accomplished by scaling the data. `min`, along with `max` and the number of bits of the encoded type, is used to determine the scaling factor. `min` is ignored if the encoded type is floating, signed, bitfield, or string. If `min` is not given, but `max` is, then `min` is assumed to be 0. `min` can be input as a mathematical expression in infix notation. For example -10000/2^15 would be correctly evaluated as -.30517578125. In addition the special strings "pi" and "e" are allowed, and will be replaced with their correct values. For example 180/pi would be evaluated as 57.295779513082321.

- `max` : The maximum value that can be encoded. `max` is ignored if the encoded type is floating, bitfield or string. If the encoded type is signed, then the minimum encoded value is `-max`. If the encoded type is unsigned, then the minimum value is `min` (or 0 if `min` is not given). If `max` or `scaer` are not given then the in memory data are not scaled, but simply cast to the encoded type. `max` can be input as a mathematical expression in the same way as `min`.

- `scaler` : The scaler that is multiplied by the in-memory type to convert to the encoded type. `scaler` is ignored if `max` is present. `scaler` and `max` (along with `min`) are different ways to represent the same thing. For signed encoded types `scaler` is converted to `max` by: `((2^(numbits-1) - 1)/scaler`. For unsigned encoded types `scaler` is converted to `max` by: `min + ((2^numbits)-1)/scaler`. `scaler` is ignored if the encoded type is floating, bitfield or string. If `scaler` or `max` are not given then the in memory data are not scaled, but simply cast to the encoded type. `scaler` can be input as a mathematical expression in the same way as `min`.

- `default` : The default value for a data. The default value is used if the received packet length is not long enough to encode all the Data. Defaults can only be used as the last element(s) of a packet. Using defaults it is possible to augment a previously defined packet in a backwards compatible way, by extending the length of the packet and adding new fields with default values so that older packets can still be interpreted.

- `constant` : is used to specify that this field in a packet is to always encoded with the same value. This is useful for encodings such as key-length-value which require specific a-prior known values to appear before the data. It can also be useful for transmitting constants such as the API of the protocol. If the encoded type is string then the constant value is interpreted as a string literal (i.e. quotes are placed around it), unless the constant value contains "(" and ")", in which case it is interpreted as a function or macro and no quotes are placed around it.

- `comment` : A one line doxygen comment that follows the data declaration.

---

The generated top level code
============================

ProtoGen will create a header file with the same name as the protocol. This module will defines any enumeration that is a direct child of the Protocol tag. Most commonly this will be an enumerated list of packet identifiers. It will include any files that the protocol xml indicates (again that is not a sub of a packet definition). Since the generated protocol header file is included in every structure and packet file this implies that the files included here will also be part of every other header. You do not have to add any include files. However it is common that an include file is used for things like array enumerations (X, Y, Z, N3D; for example). If you do not want the protocol ICD to define these enumerations, but you do want to use them in the packet definitions, then including a file is the easiest way to achieve that. The top level module will also provide functions to retrieve information about the protocol. These functions are only included if the protocol ICD includes API or Version information:

    //! Return the protocol API enumeration
    int getDemolinkApi(void);

    //! Return the protocol version string
    const char* getDemolinkVersion(void);

In the example above the string "Demolink" is the name of the protocol and comes from the protocol xml.

Generic Packets
---------------

Despite the name ProtoGen is not really a protocol or packet generator. It is instead an encode/decode generator. ProtoGen does not know (or care) about specific packet details. This is by design. The low level packet routines do not often change and therefore do not realize the same auto-coding advantages as the encoding/decoding code. ProtoGen assumes the following about packets: Packets transport a hunk of data which is internally represented as an array of bytes. Packets have an identifier number that designates their function. Packets have a size, which describes the number of bytes of data they transport. These assumptions are represented by five functions whose prototypes are created by ProtoGen and which the generated code calls (you provide the implementation):

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

Each function takes a pointer to a packet. The implementation of the function will cast this void pointer to the correct packet structure type. When the generated encode routine encodes data into a packet it will first call `getDemolinkPacketData()` to retrieve a pointer to the start of the packets data payload. When the generated code finishes the data encoding it will call `finishProtocolPacket()`, which can then perform any work to complete the packet (such as filling out the header and or generating the CRC). In the reverse direction the decode routine checks a packets ID by comparing the return of `getDemolinkPacketID()` with the ID indicated by the protocol ICD. The decode routine will also verify the packet size meets the minimum requirements.

Some packet interfaces use multiple ID values. An example would be the uBlox GPS protocol, each packet of which includes a "group" as well as an "ID". For this reason ProtoGen gives 32-bits for the ID value. The intent is that multiple IDs (which are typically less than 32-bits) can be concatenated into a single value for ProtoGens purposes.  

The generated packet code
=========================

The packet structure
--------------------

Each packet defined in the protocol xml will produce code (typically in its own source and header files) that defines a structure to represent the data-in-memory that the packet transports. In some implementations you may write interface glue code to copy the actual in memory data to this structure before passing it to the generated routines. In other implementations the structures defined by ProtoGen will be used directly in the rest of the project. This is the intended use case and is the purpose of defining separate "in memory" and "encoded" data types. You can use whatever in memory data type best fits your use case without worrying (much) about the impact on the protocol efficiency. The structure defined by a packets autogenerated code will have a typedef name like "prefixPacket_t" where "prefix" is the protocol prefix (if any) given in the xml and "Packet" is the name of the packet from the xml.

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

The autogenerated code will define any named enumeration given in the protocol xml for the packet. It will define functions to determine the packet ID and the minimum length of the encoded data of the packet. The packet lengths are given in numbers of bytes. ProtoGen refers to "minimum length", rather than "length" because packets can include default data, variable length arrays, or strings, any of which can cause the packet length to vary.

Variable length packets are a great tool for optimizing bandwidth utilization without limiting the in-memory capabilities of the code. However the correct length of a variable length packet cannot be determined until the packet has been mostly decoded; therefore the autogenerated code will check the length of such a packet twice. The first check occurs before any decoding is done to verify the packet meets the minimum length. The second check occurs when the packet decoding is complete (or only default fields are left) to verify that there were enough bytes to complete the variable length fields.

    //! return the minimum data length for the  Version packet
    int getVersionMinDataLength(void);

    //! return the packet ID for the  Version packet
    uint32_t getVersionPacketID(void);

Packet encoding and decoding functions
------------------------------------------

The autogenerated code will define functions to encode and decode the packets. These functions convert the in-memory representation to and from the encoded representation. These are the functions that your code will most likely interface with. Notice that there are two forms to the packet functions. The form that ends in "Structure" takes a pointer to a structure (`Version_t` in this example) defined by this packets module. The other form allows you to specify individual data fields as parameters. Which form you use is up to you based on how you want to keep the data in memory. Note that the parameter interfaced function will not be output unless the `parameterInterface` attribute is set to `true`.

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

The packet is allocated by the caller and passed in via void pointer (pkt). Using void pointer means that the generated code does not need to know anything about the low level packet details. See the section on Generic Packets for the means by which the generated code will interface to the packet definition. Notice that the decode function returns `int`. If you attempt to decode a packet whose ID does not match the xml description, or whose size is not sufficient, then `0` will be returned. In that case the `user` structure will be unchanged if the packet is a fixed length; but if the packet is variable length the structure may have been modified with bogus data. Hence the return value from the decode function *must* be checked.

Structure encoding and decoding functions
---------------------------------------------

In addition to the packet functions the autogenerated code can include functions to encode and decode a structure to a byte array. These functions can be used if you do not want the generated code to interact with packet routines. Perhaps because your data are not being encoded in packets, or the simple packet interfaces that ProtoGen supports are not sophisticated enough. These functions will only be generated for Structure tags, not Packet tags. If you want to simultaneously have a structure with generic encoding functions and packet functions; then create both a structure tag and a packet tag which references the structure (see the GPS example).

    //! Encode a Version_t structure into a byte array
    int encodeVersion_t(uint8_t* data, int byteCount, const Version_t* user);

    //! Decode a Version_t structure from a byte array
    int decodeVersion_t(const uint8_t* data, int byteCount, Version_t* user);

Other generated code
====================

ProtoGen also creates other files that are not specified by the xml, but are used as helper functions for the generated packet code. These are the modules: bitfieldspecial, floatspecial, fieldencode, fielddecode, scaledencode, scaleddecode. Although these modules are not specified by the xml they are still auto-generated. Much of the code in these modules is tedious and repetitive, so it was ultimatley simpler and less error prone to auto generate it. More importantly automatically generating this code makes it easier for future versions of ProtoGen to take advantage of changes or advances in the routines these modules provide.

floatspecial
------------

floatspecial provides routines to convert to and from normal floating point types (64 and 32 bit) to the special types also supported by ProtoGen (namely, 16 and 24 bits). One could legitamately ask why such types are needed: isn't it better to simply scale a 32 (or 64) bit float down to a smaller integer? Usually yes, but there are cases where this is not ideal. A good example is the amount of fuel on board an aircraft: Typically the accuracy of the fuel level estimate or sensor is no more than 8 bits. However the dynamic range of the fuel level is a function of the aircraft type. A 4 pound drone might carry 1 pound of fuel. An A380 airliner might carry 500,000 lbs of fuel. There is no simple way to scale the in memory floating point fuel level to a smaller integer type without a-priori knowing the size of vehicle (or using an excessive number of bits). However a 16-bit floating point type with 9 bits of resolution can do this nicely. So for that case, supporting a 16-bit float allows us to save 2 bytes in the encoded data stream. The 16 and 24 bit float formats use the same layout as IEEE-754: the most signficant bit is a sign bit, the next bits are the biased exponent, the least significant bits are the significand with an implied leading 1. The 24-bit format uses the same number of exponent bits as float32 (8 bits of exponent). Hence float24 covers the same range as a float32, but with 15 bits of resolution rather than 23 bits. The 16-bit format uses 6 bits for exponent, so it has a range that is 1/4 of float32 (aproximately -2^31 to 2^31), but with 9 bits of resolution rather than 23 bits.

floatspecial also provides routines to determine if a pattern of 32 bits is a valid `float`, and if a pattern of 64 bits is a valid `double`. In the case where a native floating point type is decoded directly from the byte stream (as opposed to being scaled from integer) these functions are used to make sure the floating point number is not infinity, NaN, or denormalized prior to loading the value into a floating point register. This is important for many embedded processors which have limited floating point environments that will throw an exception in the event of an invalid floating point. Any invalid floating point that is decoded is replaced with 0.

ProtoGen assumes that the `float` (32-bit) and `double` (64-bit) types adhere to IEEE-754. ProtoGen's assumption of the layout of the `float` and `double` types is only a factor in two cases: 1) if the protocol you specify uses 16 or 24 bit floating point types (i.e. if a conversion between the types is needed) and 2) if a native 32 or 64 bit float type is encoded without scaling by integer, which will trigger the check to determine if the float is valid when it is decoded. If any of your processors do not adhere to the IEEE-754 spec for floating point, do not use 16 or 24 bit floats in your protocol ICD. If you set the protocol attribute `supportSpecialFloat="false"` then the floatspecial module will not be emitted and any reference to float16 or float24 in the protocol will generate a warning. In addition setting `supportSpecialFloat="false"` will cause ProtoGen to skip the valid float check on decode.

bitfieldspecial
---------------

bitfieldspecial provides routines for encoding and decoding bitfields into and out of byte arrays. If you set the protocol attribute `supportBitfield="false"` then this file will not be output. In addition any bitfields in the protocol description will generated a warning, and the field will be converted to the next larger in-memory unsigned integer.

fieldencode and fielddecode
---------------------------

fieldencode does the work of moving data from a native type to an array of bytes. This is not as simple as you might think. In the simplest case one could simply copy the data from one memory location to another (for example, using `memcpy()`); however that would ignore the complications caused by local byte order (big or little endian) and memory alignment requirements. For example, if the processor is little endian (x86) but the protocol is big endian the bytes of the native type have to be swapped. Furthermore, we do not necessarily know the alignment of the packets data pointer, so copying a four byte mis-aligned native type (for example) may not be allowed by the processor.

The way to handle both these issues is to copy the data byte by byte. There are numerous methods by which this can be done. ProtoGen does it using leftshift (`<<`) and rightshift (`>>`) operators. This has the advantage of (potentially) leaving the native type in a register during the copy, and not needing to know the local endianness. Even this operation has room for interpretation. The maximum number of bits that can be shifted is architecture dependent; but is typically the number of bits of an `int`. Hence the process of shifting the bits from the field to the data array (and vice versa) is ordered such that only 8 bit shifts are used, allowing ProtoGen code to run on 8 bit microcontrollers.

fieldencode also provides the routines to encode non native types, such as `int24_t`. `int24_t` is a 24 bit signed type, which does not exist in most computer architectures. Instead fieldencode provides routines to take a `int32_t` and encode it as a `int24_t`, by discarding the most significant byte. Routines are provided for every byte width from 1 byte to 8 bytes, for both signed and unsigned numbers. If you set the protocol attribute `supportInt64="false"` then support for integer types greater than 32 bits will be omitted. This removes a lot of functions from this module. Note that you can still encode double precision floating points in this case. To disable double precision floating points then set the protocol attribute `supportFloat64="false"`.

fielddecode provides the decoding routines that are the corollary to the routines in fieldencode. These are slightly more challenging for non-native signed types, because special code must be added to perform sign extension of such types when they are converted to the next largest native type.

scaledencode and scaleddecode
-----------------------------

scaledencode and scaleddecode provide routines to take an in memory number (like a `double` for example) and scale it to any smaller size integer. An example use case would be to take a double precision geodetic latitude in radians and encode it as a 32-bit signed integer number. Doing so would provide better than centimeter resolution for the encoded type, while using only half the space in the encoded format as the in memory format. Routines are not provided to inflate the size of an in memory type. For example there is no routine to take a float and encode it in a 64-bit integer. This would simply waste datalink bandwidth without providing any resolution improvement.

If you set the protocol attribute `supportInt64="false"` then support for integer types greater than 32 bits will be omitted. This removes a *lot* of functions from this module. Note that you can still encode scaled double precision floating points in this case (as long as you scale them to 32 bits or less). To disable double precision floating points then set the protocol attribute `supportFloat64="false"`.

Generation of documentation
===========================

Documentation for a protocol is often the last task undertaken by a developer, but arguably should be the first. The usefulness of a protocol is largely dictated by the quality of its documentation (unless you plan to be the only one to ever use it!). ProtoGen attempts to provide automatic documentation. The emitted code will be decorated with doxygen style comments. If doxygen is installed ProtoGen can use it to generate html documentation for the code. This documentation is useful for developers who are coding against the API provided by the generated code.

ProtoGen will also output a Demolink.markdown file which provides a Markdown formatted document of the protocol. ProtoGen can feed this file to Markdown (if perl is installed) in order to create a Demolink.html file. This file is intended as a master document for the protocol that is suitable for developers and users alike. The author of the protocol xml is encouraged to be verbose in the comment attributes. Remember: the better you document your protocol the less users will bug you for help!

---

About the author
===========================

Contact information:

Bill Vaglienti

Five by Five Development, LLC

[www.fivebyfivedevelopment.com](http://www.fivebyfivedevelopment.com)
 
Before founding Five by Five Development in 2013 I was co-founder of [Cloud Cap Technology](http://www.cloudcaptech.com), which developed the Piccolo flight management system. Although Piccolo was at heart a flight controller, I spent more time coding communications protocols than anything else. Hopefully ProtoGen will allow myself and others to spend less time doing that.
