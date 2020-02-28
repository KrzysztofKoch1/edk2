/** @file
  Header file for ACPI parser

  Copyright (c) 2016 - 2020, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ACPIPARSER_H_
#define ACPIPARSER_H_

#define OUTPUT_FIELD_COLUMN_WIDTH  36

/// The RSDP table signature is "RSD PTR " (8 bytes)
/// However The signature for ACPI tables is 4 bytes.
/// To work around this oddity define a signature type
/// that allows us to process the log options.
#define RSDP_TABLE_INFO  SIGNATURE_32('R', 'S', 'D', 'P')

/**
  This function increments the ACPI table error counter.
**/
VOID
EFIAPI
IncrementErrorCount (
  VOID
  );

/**
  This function increments the ACPI table warning counter.
**/
VOID
EFIAPI
IncrementWarningCount (
  VOID
  );

/**
  This function verifies the ACPI table checksum.

  This function verifies the checksum for the ACPI table and optionally
  prints the status.

  @param [in] Log     If TRUE log the status of the checksum.
  @param [in] Ptr     Pointer to the start of the table buffer.
  @param [in] Length  The length of the buffer.

  @retval TRUE         The checksum is OK.
  @retval FALSE        The checksum failed.
**/
BOOLEAN
EFIAPI
VerifyChecksum (
  IN BOOLEAN Log,
  IN UINT8*  Ptr,
  IN UINT32  Length
  );

/**
  This function performs a raw data dump of the ACPI table.

  @param [in] Ptr     Pointer to the start of the table buffer.
  @param [in] Length  The length of the buffer.
**/
VOID
EFIAPI
DumpRaw (
  IN UINT8* Ptr,
  IN UINT32 Length
  );

/**
  This function traces 1 byte of datum as specified in the format string.

  @param [in] Format  The format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
VOID
EFIAPI
DumpUint8 (
  IN CONST CHAR16* Format,
  IN UINT8*        Ptr
  );

/**
  This function traces 2 bytes of data as specified in the format string.

  @param [in] Format  The format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
VOID
EFIAPI
DumpUint16 (
  IN CONST CHAR16* Format,
  IN UINT8*        Ptr
  );

/**
  This function traces 4 bytes of data as specified in the format string.

  @param [in] Format  The format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
VOID
EFIAPI
DumpUint32 (
  IN CONST CHAR16* Format,
  IN UINT8*        Ptr
  );

/**
  This function traces 8 bytes of data as specified by the format string.

  @param [in] Format  The format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
VOID
EFIAPI
DumpUint64 (
  IN CONST CHAR16* Format,
  IN UINT8*        Ptr
  );

/**
  This function traces 3 characters which can be optionally
  formated using the format string if specified.

  If no format string is specified the Format must be NULL.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
VOID
EFIAPI
Dump3Chars (
  IN CONST CHAR16* Format OPTIONAL,
  IN UINT8*        Ptr
  );

/**
  This function traces 4 characters which can be optionally
  formated using the format string if specified.

  If no format string is specified the Format must be NULL.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
VOID
EFIAPI
Dump4Chars (
  IN CONST CHAR16* Format OPTIONAL,
  IN UINT8*        Ptr
  );

/**
  This function traces 6 characters which can be optionally
  formated using the format string if specified.

  If no format string is specified the Format must be NULL.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
VOID
EFIAPI
Dump6Chars (
  IN CONST CHAR16* Format OPTIONAL,
  IN UINT8*        Ptr
  );

/**
  This function traces 8 characters which can be optionally
  formated using the format string if specified.

  If no format string is specified the Format must be NULL.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
VOID
EFIAPI
Dump8Chars (
  IN CONST CHAR16* Format OPTIONAL,
  IN UINT8*        Ptr
  );

/**
  This function traces 12 characters which can be optionally
  formated using the format string if specified.

  If no format string is specified the Format must be NULL.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
VOID
EFIAPI
Dump12Chars (
  IN CONST CHAR16* Format OPTIONAL,
  IN       UINT8*  Ptr
  );

/**
  This function indents and prints the ACPI table Field Name.

  @param [in] Indent      Number of spaces to add to the global table
                          indent. The global table indent is 0 by default;
                          however this value is updated on entry to the
                          ParseAcpi() by adding the indent value provided to
                          ParseAcpi() and restored back on exit. Therefore
                          the total indent in the output is dependent on from
                          where this function is called.
  @param [in] FieldName   Pointer to the Field Name.
**/
VOID
EFIAPI
PrintFieldName (
  IN UINT32         Indent,
  IN CONST CHAR16*  FieldName
  );

/**
  This function pointer is the template for customizing the trace output

  @param [in] Format  Format string for tracing the data as specified by
                      the 'Format' member of ACPI_PARSER.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
typedef VOID (EFIAPI *FNPTR_PRINT_FORMATTER)(CONST CHAR16* Format, UINT8* Ptr);

/**
  This function pointer is the template for validating an ACPI table field.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information as specified by
                      the 'Context' member of the ACPI_PARSER.
                      e.g. this could be a pointer to the ACPI table header.
**/
typedef VOID (EFIAPI *FNPTR_FIELD_VALIDATOR)(UINT8* Ptr, VOID* Context);

/**
  The ACPI_PARSER structure describes the fields of an ACPI table and
  provides means for the parser to interpret and trace appropriately.

  The first three members are populated based on information present in
  in the ACPI table specifications. The remaining members describe how
  the parser should report the field information, validate the field data
  and/or update an external pointer to the field (ItemPtr).

  ParseAcpi() uses the format string specified by 'Format' for tracing
  the field data. If the field is more complex and requires additional
  processing for formatting and representation a print formatter function
  can be specified in 'PrintFormatter'.
  The PrintFormatter function may choose to use the format string
  specified by 'Format' or use its own internal format string.

  The 'Format' and 'PrintFormatter' members allow flexibility for
  representing the field data.
**/
typedef struct AcpiParser {

  /// String describing the ACPI table field
  /// (Field column from ACPI table spec)
  CONST CHAR16*         NameStr;

  /// The length of the field.
  /// (Byte Length column from ACPI table spec)
  UINT32                Length;

  /// The offset of the field from the start of the table.
  /// (Byte Offset column from ACPI table spec)
  UINT32                Offset;

  /// Optional Print() style format string for tracing the data. If not
  /// used this must be set to NULL.
  CONST CHAR16*         Format;

  /// Optional pointer to a print formatter function which
  /// is typically used to trace complex field information.
  /// If not used this must be set to NULL.
  /// The Format string is passed to the PrintFormatter function
  /// but may be ignored by the implementation code.
  FNPTR_PRINT_FORMATTER PrintFormatter;

  /// Optional pointer which may be set to request the parser to update
  /// a pointer to the field data. If unused this must be set to NULL.
  VOID**                ItemPtr;

  /// Optional pointer to a field validator function.
  /// The function should directly report any appropriate error or warning
  /// and invoke the appropriate counter update function.
  /// If not used this parameter must be set to NULL.
  FNPTR_FIELD_VALIDATOR FieldValidator;

  /// Optional pointer to context specific information,
  /// which the Field Validator function can use to determine
  /// additional information about the ACPI table and make
  /// decisions about the field being validated.
  /// e.g. this could be a pointer to the ACPI table header
  VOID*                 Context;
} ACPI_PARSER;

/**
  Produce a Null-terminated ASCII string with the name and index of an
  ACPI structure.

  The output string is in the following format: <Name> [<Index>]

  @param [in]  Name           Structure name.
  @param [in]  Index          Structure index.
  @param [in]  BufferSize     The size, in bytes, of the output buffer.
  @param [out] Buffer         Buffer for the output string.

  @return   The number of bytes written to the buffer (not including Null-byte)
**/
UINTN
EFIAPI
PrintAcpiStructName (
  IN  CONST CHAR8*  Name,
  IN        UINT32  Index,
  IN        UINTN   BufferSize,
  OUT       CHAR8*  Buffer
  );

/**
  Indentation for printing instance counts for structures in an ACPI table.
**/
#define INSTANCE_COUNT_INDENT 2

/**
  Common signature for functions which parse ACPI structures.

  @param [in] Ptr         Pointer to the start of structure's buffer.
  @param [in] Length      Length of the buffer.
  @param [in] OptArg0     First optional argument.
  @param [in] OptArg1     Second optional argument.
*/
typedef VOID (*ACPI_STRUCT_PARSER_FUNC) (
  IN       UINT8* Ptr,
  IN       UINT32 Length,
  IN CONST VOID*  OptArg0 OPTIONAL,
  IN CONST VOID*  OptArg1 OPTIONAL
  );

/**
  Description of how an ACPI structure should be parsed.

  One of ParserFunc or ParserArray should be not NULL. Otherwise, it is
  assumed that parsing of an ACPI structure is not supported. If both
  ParserFunc and ParserArray are defined, ParserFunc is used.
**/
typedef struct AcpiStructHandler {
  /// Dedicated function for parsing an ACPI structure
  ACPI_STRUCT_PARSER_FUNC   ParserFunc;
  /// Array of instructions on how each structure field should be parsed
  CONST ACPI_PARSER*        ParserArray;
  /// Number of elements in ParserArray if ParserArray is defined
  UINT32                    Elements;
} ACPI_STRUCT_HANDLER;

/**
  ACPI structure compatiblity with various architectures.

  Some ACPI tables define structures which are, for example, only valid in
  the X64 or Arm context. For instance, the Multiple APIC Description Table
  (MADT) describes both APIC and GIC interrupt models.

  These definitions provide means to describe the belonging of a structure
  in an ACPI table to a particular architecture. This way, incompatible
  structures can be detected.
**/
#define ARCH_COMPAT_IA32       BIT0
#define ARCH_COMPAT_X64        BIT1
#define ARCH_COMPAT_ARM        BIT2
#define ARCH_COMPAT_AARCH64    BIT3
#define ARCH_COMPAT_RISCV64    BIT4

/**
  Information about a structure which constitutes an ACPI table
**/
typedef struct AcpiStructInfo {
  /// ACPI-defined structure Name
  CONST CHAR8*                Name;
  /// ACPI-defined structure Type
  CONST UINT32                Type;
  /// Architecture(s) for which this structure is valid
  CONST UINT32                CompatArch;
  /// Structure's instance count in a table
  UINT32                      Count;
  /// Information on how to handle the structure
  CONST ACPI_STRUCT_HANDLER   Handler;
} ACPI_STRUCT_INFO;

/**
  Macro for defining ACPI structure info when an ACPI_PARSER array must
  be used to parse the structure.
**/
#define ADD_ACPI_STRUCT_INFO_ARRAY(Name, Type, Compat, Array)             \
{                                                                         \
  Name, Type, Compat, 0, {NULL, Array, ARRAY_SIZE (Array)}                \
}

/**
  Macro for defining ACPI structure info when an ACPI_STRUCT_PARSER_FUNC
  must be used to parse the structure.
**/
#define ADD_ACPI_STRUCT_INFO_FUNC(Name, Type, Compat, Func)               \
{                                                                         \
  Name, Type, Compat, 0, {Func, NULL, 0}                                  \
}

/**
  Macro for defining ACPI structure info when the structure is defined in
  the ACPI spec but no parsing information is provided.
**/
#define ACPI_STRUCT_INFO_PARSER_NOT_IMPLEMENTED(Name, Type, Compat)       \
{                                                                         \
  Name, Type, Compat, 0, {NULL, NULL, 0}                                  \
}

/**
  Database collating information about every structure type defined by
  an ACPI table.
**/
typedef struct AcpiStructDatabase {
  /// ACPI-defined name for the structures being described in the database
  CONST CHAR8*        Name;
  /// Per-structure-type information. The list must be ordered by the types
  /// defined for the table. All entries must be unique and there should be
  /// no gaps.
  ACPI_STRUCT_INFO*   Entries;
  /// Total number of unique types defined for the table
  CONST UINT32        EntryCount;
} ACPI_STRUCT_DATABASE;

/**
  Set all ACPI structure instance counts to 0.

  @param [in,out] StructDb     ACPI structure database with counts to reset.
**/
VOID
EFIAPI
ResetAcpiStructCounts (
  IN OUT ACPI_STRUCT_DATABASE* StructDb
  );

/**
  Sum all ACPI structure instance counts.

  @param [in] StructDb     ACPI structure database with per-type counts to sum.

  @return   Total number of structure instances recorded in the database.
**/
UINT32
EFIAPI
SumAcpiStructCounts (
  IN  CONST ACPI_STRUCT_DATABASE* StructDb
  );

/**
  Validate that a structure with a given type value is defined for the given
  ACPI table and target architecture.

  The target architecture is evaluated from the firmare build parameters.

  @param [in] Type        ACPI-defined structure type.
  @param [in] StructDb    ACPI structure database with architecture
                          compatibility info.

  @retval TRUE    Structure is valid.
  @retval FALSE   Structure is not valid.
**/
BOOLEAN
EFIAPI
IsAcpiStructTypeValid (
  IN        UINT32                Type,
  IN  CONST ACPI_STRUCT_DATABASE* StructDb
  );

/**
  Print the instance count of each structure in an ACPI table that is
  compatible with the target architecture.

  For structures which are not allowed for the target architecture,
  validate that their instance counts are 0.

  @param [in] StructDb     ACPI structure database with counts to validate.

  @retval TRUE    All structures are compatible.
  @retval FALSE   One or more incompatible structures present.
**/
BOOLEAN
EFIAPI
ValidateAcpiStructCounts (
  IN  CONST ACPI_STRUCT_DATABASE* StructDb
  );

/**
  Parse the ACPI structure with the type value given according to instructions
  defined in the ACPI structure database.

  If the input structure type is defined in the database, increment structure's
  instance count.

  If ACPI_PARSER array is used to parse the input structure, the index of the
  structure (instance count for the type before update) gets printed alongside
  the structure name. This helps debugging if there are many instances of the
  type in a table. For ACPI_STRUCT_PARSER_FUNC, the printing of the index must
  be implemented separately.

  @param [in]     Indent    Number of spaces to indent the output.
  @param [in]     Ptr       Ptr to the start of the structure.
  @param [in,out] StructDb  ACPI structure database with instructions on how
                            parse every structure type.
  @param [in]     Offset    Structure offset from the start of the table.
  @param [in]     Type      ACPI-defined structure type.
  @param [in]     Length    Length of the structure in bytes.
  @param [in]     OptArg0   First optional argument to pass to parser function.
  @param [in]     OptArg1   Second optional argument to pass to parser function.

  @retval TRUE    ACPI structure parsed successfully.
  @retval FALSE   Undefined structure type or insufficient data to parse.
**/
BOOLEAN
EFIAPI
ParseAcpiStruct (
  IN            UINT32                 Indent,
  IN            UINT8*                 Ptr,
  IN OUT        ACPI_STRUCT_DATABASE*  StructDb,
  IN            UINT32                 Offset,
  IN            UINT32                 Type,
  IN            UINT32                 Length,
  IN      CONST VOID*                  OptArg0 OPTIONAL,
  IN      CONST VOID*                  OptArg1 OPTIONAL
  );

/**
  A structure used to store the pointers to the members of the
  ACPI description header structure that was parsed.
**/
typedef struct AcpiDescriptionHeaderInfo {
  /// ACPI table signature
  UINT32* Signature;
  /// Length of the ACPI table
  UINT32* Length;
  /// Revision
  UINT8*  Revision;
  /// Checksum
  UINT8*  Checksum;
  /// OEM Id - length is 6 bytes
  UINT8*  OemId;
  /// OEM table Id
  UINT64* OemTableId;
  /// OEM revision Id
  UINT32* OemRevision;
  /// Creator Id
  UINT32* CreatorId;
  /// Creator revision
  UINT32* CreatorRevision;
} ACPI_DESCRIPTION_HEADER_INFO;

/**
  This function is used to parse an ACPI table buffer.

  The ACPI table buffer is parsed using the ACPI table parser information
  specified by a pointer to an array of ACPI_PARSER elements. This parser
  function iterates through each item on the ACPI_PARSER array and logs the
  ACPI table fields.

  This function can optionally be used to parse ACPI tables and fetch specific
  field values. The ItemPtr member of the ACPI_PARSER structure (where used)
  is updated by this parser function to point to the selected field data
  (e.g. useful for variable length nested fields).

  @param [in] Trace        Trace the ACPI fields TRUE else only parse the
                           table.
  @param [in] Indent       Number of spaces to indent the output.
  @param [in] AsciiName    Optional pointer to an ASCII string that describes
                           the table being parsed.
  @param [in] Ptr          Pointer to the start of the buffer.
  @param [in] Length       Length of the buffer pointed by Ptr.
  @param [in] Parser       Pointer to an array of ACPI_PARSER structure that
                           describes the table being parsed.
  @param [in] ParserItems  Number of items in the ACPI_PARSER array.

  @retval Number of bytes parsed.
**/
UINT32
EFIAPI
ParseAcpi (
  IN BOOLEAN            Trace,
  IN UINT32             Indent,
  IN CONST CHAR8*       AsciiName OPTIONAL,
  IN UINT8*             Ptr,
  IN UINT32             Length,
  IN CONST ACPI_PARSER* Parser,
  IN UINT32             ParserItems
  );

/**
   This is a helper macro to pass parameters to the Parser functions.

  @param [in] Parser The name of the ACPI_PARSER array describing the
              ACPI table fields.
**/
#define PARSER_PARAMS(Parser) Parser, sizeof (Parser) / sizeof (Parser[0])

/**
  This is a helper macro for describing the ACPI header fields.

  @param [out] Info  Pointer to retrieve the ACPI table header information.
**/
#define PARSE_ACPI_HEADER(Info)                   \
  { L"Signature", 4, 0, NULL, Dump4Chars,         \
    (VOID**)&(Info)->Signature , NULL, NULL },    \
  { L"Length", 4, 4, L"%d", NULL,                 \
    (VOID**)&(Info)->Length, NULL, NULL },        \
  { L"Revision", 1, 8, L"%d", NULL,               \
    (VOID**)&(Info)->Revision, NULL, NULL },      \
  { L"Checksum", 1, 9, L"0x%X", NULL,             \
    (VOID**)&(Info)->Checksum, NULL, NULL },      \
  { L"Oem ID", 6, 10, NULL, Dump6Chars,           \
    (VOID**)&(Info)->OemId, NULL, NULL },         \
  { L"Oem Table ID", 8, 16, NULL, Dump8Chars,     \
    (VOID**)&(Info)->OemTableId, NULL, NULL },    \
  { L"Oem Revision", 4, 24, L"0x%X", NULL,        \
    (VOID**)&(Info)->OemRevision, NULL, NULL },   \
  { L"Creator ID", 4, 28, NULL, Dump4Chars,       \
    (VOID**)&(Info)->CreatorId, NULL, NULL },     \
  { L"Creator Revision", 4, 32, L"0x%X", NULL,    \
    (VOID**)&(Info)->CreatorRevision, NULL, NULL }

/**
  This function indents and traces the GAS structure as described by the GasParser.

  @param [in] Ptr     Pointer to the start of the buffer.
  @param [in] Indent  Number of spaces to indent the output.
  @param [in] Length  Length of the GAS structure buffer.

  @retval Number of bytes parsed.
**/
UINT32
EFIAPI
DumpGasStruct (
  IN UINT8*        Ptr,
  IN UINT32        Indent,
  IN UINT32        Length
  );

/**
  This function traces the GAS structure as described by the GasParser.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
VOID
EFIAPI
DumpGas (
  IN CONST CHAR16* Format OPTIONAL,
  IN UINT8*        Ptr
  );

/**
  This function traces the ACPI header as described by the AcpiHeaderParser.

  @param [in] Ptr          Pointer to the start of the buffer.

  @retval Number of bytes parsed.
**/
UINT32
EFIAPI
DumpAcpiHeader (
  IN UINT8* Ptr
  );

/**
  This function parses the ACPI header as described by the AcpiHeaderParser.

  This function optionally returns the Signature, Length and revision of the
  ACPI table.

  @param [in]  Ptr        Pointer to the start of the buffer.
  @param [out] Signature  Gets location of the ACPI table signature.
  @param [out] Length     Gets location of the length of the ACPI table.
  @param [out] Revision   Gets location of the revision of the ACPI table.

  @retval Number of bytes parsed.
**/
UINT32
EFIAPI
ParseAcpiHeader (
  IN  UINT8*         Ptr,
  OUT CONST UINT32** Signature,
  OUT CONST UINT32** Length,
  OUT CONST UINT8**  Revision
  );

/**
  This function parses the ACPI BGRT table.
  When trace is enabled this function parses the BGRT table and
  traces the ACPI table fields.

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiBgrt (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/**
  This function parses the ACPI DBG2 table.
  When trace is enabled this function parses the DBG2 table and
  traces the ACPI table fields.

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiDbg2 (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/**
  This function parses the ACPI DSDT table.
  When trace is enabled this function parses the DSDT table and
  traces the ACPI table fields.
  For the DSDT table only the ACPI header fields are parsed and
  traced.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiDsdt (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/**
  This function parses the ACPI FACS table.
  When trace is enabled this function parses the FACS table and
  traces the ACPI table fields.

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiFacs (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/**
  This function parses the ACPI FADT table.
  This function parses the FADT table and optionally traces the ACPI
  table fields.

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiFadt (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/**
  This function parses the ACPI GTDT table.
  When trace is enabled this function parses the GTDT table and
  traces the ACPI table fields.

  This function also parses the following platform timer structures:
    - GT Block timer
    - Watchdog timer

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiGtdt (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/**
  This function parses the ACPI IORT table.
  When trace is enabled this function parses the IORT table and
  traces the ACPI fields.

  This function also parses the following nodes:
    - ITS Group
    - Named Component
    - Root Complex
    - SMMUv1/2
    - SMMUv3
    - PMCG

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiIort (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/**
  This function parses the ACPI MADT table.
  When trace is enabled this function parses the MADT table and
  traces the ACPI table fields.

  This function currently parses the following Interrupt Controller
  Structures:
    - GICC
    - GICD
    - GIC MSI Frame
    - GICR
    - GIC ITS

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiMadt (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/**
  This function parses the ACPI MCFG table.
  When trace is enabled this function parses the MCFG table and
  traces the ACPI table fields.

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiMcfg (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/**
  This function parses the ACPI PPTT table.
  When trace is enabled this function parses the PPTT table and
  traces the ACPI table fields.

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiPptt (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/**
  This function parses the ACPI RSDP table.

  This function invokes the parser for the XSDT table.
  * Note - This function does not support parsing of RSDT table.

  This function also performs a RAW dump of the ACPI table and
  validates the checksum.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiRsdp (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/**
  This function parses the ACPI SLIT table.
  When trace is enabled this function parses the SLIT table and
  traces the ACPI table fields.

  This function also validates System Localities for the following:
    - Diagonal elements have a normalized value of 10
    - Relative distance from System Locality at i*N+j is same as
      j*N+i

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiSlit (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/**
  This function parses the ACPI SPCR table.
  When trace is enabled this function parses the SPCR table and
  traces the ACPI table fields.

  This function also performs validations of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiSpcr (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/**
  This function parses the ACPI SRAT table.
  When trace is enabled this function parses the SRAT table and
  traces the ACPI table fields.

  This function parses the following Resource Allocation Structures:
    - Processor Local APIC/SAPIC Affinity Structure
    - Memory Affinity Structure
    - Processor Local x2APIC Affinity Structure
    - GICC Affinity Structure

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiSrat (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/**
  This function parses the ACPI SSDT table.
  When trace is enabled this function parses the SSDT table and
  traces the ACPI table fields.
  For the SSDT table only the ACPI header fields are
  parsed and traced.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiSsdt (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/**
  This function parses the ACPI XSDT table
  and optionally traces the ACPI table fields.

  This function also performs validation of the XSDT table.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiXsdt (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

#endif // ACPIPARSER_H_
