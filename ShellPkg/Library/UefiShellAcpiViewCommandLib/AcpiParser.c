/** @file
  ACPI parser

  Copyright (c) 2016 - 2020, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include "AcpiParser.h"
#include "AcpiView.h"

STATIC UINT32   gIndent;
STATIC UINT32   mTableErrorCount;
STATIC UINT32   mTableWarningCount;

STATIC ACPI_DESCRIPTION_HEADER_INFO AcpiHdrInfo;

/**
  An ACPI_PARSER array describing the ACPI header.
**/
STATIC CONST ACPI_PARSER AcpiHeaderParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo)
};

/**
  This function resets the ACPI table error counter to Zero.
**/
VOID
ResetErrorCount (
  VOID
  )
{
  mTableErrorCount = 0;
}

/**
  This function returns the ACPI table error count.

  @retval Returns the count of errors detected in the ACPI tables.
**/
UINT32
GetErrorCount (
  VOID
  )
{
  return mTableErrorCount;
}

/**
  This function resets the ACPI table warning counter to Zero.
**/
VOID
ResetWarningCount (
  VOID
  )
{
  mTableWarningCount = 0;
}

/**
  This function returns the ACPI table warning count.

  @retval Returns the count of warning detected in the ACPI tables.
**/
UINT32
GetWarningCount (
  VOID
  )
{
  return mTableWarningCount;
}

/**
  This function increments the ACPI table error counter.
**/
VOID
EFIAPI
IncrementErrorCount (
  VOID
  )
{
  mTableErrorCount++;
}

/**
  This function increments the ACPI table warning counter.
**/
VOID
EFIAPI
IncrementWarningCount (
  VOID
  )
{
  mTableWarningCount++;
}

/**
  This function verifies the ACPI table checksum.

  This function verifies the checksum for the ACPI table and optionally
  prints the status.

  @param [in] Log     If TRUE log the status of the checksum.
  @param [in] Ptr     Pointer to the start of the table buffer.
  @param [in] Length  The length of the buffer.

  @retval TRUE        The checksum is OK.
  @retval FALSE       The checksum failed.
**/
BOOLEAN
EFIAPI
VerifyChecksum (
  IN BOOLEAN Log,
  IN UINT8*  Ptr,
  IN UINT32  Length
  )
{
  UINTN ByteCount;
  UINT8 Checksum;
  UINTN OriginalAttribute;

  //
  // set local variables to suppress incorrect compiler/analyzer warnings
  //
  OriginalAttribute = 0;
  ByteCount = 0;
  Checksum = 0;

  while (ByteCount < Length) {
    Checksum += *(Ptr++);
    ByteCount++;
  }

  if (Log) {
    OriginalAttribute = gST->ConOut->Mode->Attribute;
    if (Checksum == 0) {
      if (GetColourHighlighting ()) {
        gST->ConOut->SetAttribute (
                       gST->ConOut,
                       EFI_TEXT_ATTR (EFI_GREEN,
                         ((OriginalAttribute&(BIT4|BIT5|BIT6))>>4))
                       );
      }
      Print (L"Table Checksum : OK\n\n");
    } else {
      IncrementErrorCount ();
      if (GetColourHighlighting ()) {
        gST->ConOut->SetAttribute (
                       gST->ConOut,
                       EFI_TEXT_ATTR (EFI_RED,
                         ((OriginalAttribute&(BIT4|BIT5|BIT6))>>4))
                       );
      }
      Print (L"Table Checksum : FAILED (0x%X)\n\n", Checksum);
    }
    if (GetColourHighlighting ()) {
      gST->ConOut->SetAttribute (gST->ConOut, OriginalAttribute);
    }
  }

  return (Checksum == 0);
}

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
  )
{
  UINTN ByteCount;
  UINTN PartLineChars;
  UINTN AsciiBufferIndex;
  CHAR8 AsciiBuffer[17];

  ByteCount = 0;
  AsciiBufferIndex = 0;

  Print (L"Address  : 0x%p\n", Ptr);
  Print (L"Length   : %d\n", Length);

  while (ByteCount < Length) {
    if ((ByteCount & 0x0F) == 0) {
      AsciiBuffer[AsciiBufferIndex] = '\0';
      Print (L"  %a\n%08X : ", AsciiBuffer, ByteCount);
      AsciiBufferIndex = 0;
    } else if ((ByteCount & 0x07) == 0) {
      Print (L"- ");
    }

    if ((*Ptr >= ' ') && (*Ptr < 0x7F)) {
      AsciiBuffer[AsciiBufferIndex++] = *Ptr;
    } else {
      AsciiBuffer[AsciiBufferIndex++] = '.';
    }

    Print (L"%02X ", *Ptr++);

    ByteCount++;
  }

  // Justify the final line using spaces before printing
  // the ASCII data.
  PartLineChars = (Length & 0x0F);
  if (PartLineChars != 0) {
    PartLineChars = 48 - (PartLineChars * 3);
    if ((Length & 0x0F) <= 8) {
      PartLineChars += 2;
    }
    while (PartLineChars > 0) {
      Print (L" ");
      PartLineChars--;
    }
  }

  // Print ASCII data for the final line.
  AsciiBuffer[AsciiBufferIndex] = '\0';
  Print (L"  %a\n\n", AsciiBuffer);
}

/**
  This function traces 1 byte of data as specified in the format string.

  @param [in] Format  The format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
VOID
EFIAPI
DumpUint8 (
  IN CONST CHAR16* Format,
  IN UINT8*        Ptr
  )
{
  Print (Format, *Ptr);
}

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
  )
{
  Print (Format, *(UINT16*)Ptr);
}

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
  )
{
  Print (Format, *(UINT32*)Ptr);
}

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
  )
{
  // Some fields are not aligned and this causes alignment faults
  // on ARM platforms if the compiler generates LDRD instructions.
  // Perform word access so that LDRD instructions are not generated.
  UINT64 Val;

  Val = *(UINT32*)(Ptr + sizeof (UINT32));

  Val = LShiftU64(Val,32);
  Val |= (UINT64)*(UINT32*)Ptr;

  Print (Format, Val);
}

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
  )
{
  Print (
    (Format != NULL) ? Format : L"%c%c%c",
    Ptr[0],
    Ptr[1],
    Ptr[2]
    );
}

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
  )
{
  Print (
    (Format != NULL) ? Format : L"%c%c%c%c",
    Ptr[0],
    Ptr[1],
    Ptr[2],
    Ptr[3]
    );
}

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
  )
{
  Print (
    (Format != NULL) ? Format : L"%c%c%c%c%c%c",
    Ptr[0],
    Ptr[1],
    Ptr[2],
    Ptr[3],
    Ptr[4],
    Ptr[5]
    );
}

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
  )
{
  Print (
    (Format != NULL) ? Format : L"%c%c%c%c%c%c%c%c",
    Ptr[0],
    Ptr[1],
    Ptr[2],
    Ptr[3],
    Ptr[4],
    Ptr[5],
    Ptr[6],
    Ptr[7]
    );
}

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
  )
{
  Print (
    (Format != NULL) ? Format : L"%c%c%c%c%c%c%c%c%c%c%c%c",
    Ptr[0],
    Ptr[1],
    Ptr[2],
    Ptr[3],
    Ptr[4],
    Ptr[5],
    Ptr[6],
    Ptr[7],
    Ptr[8],
    Ptr[9],
    Ptr[10],
    Ptr[11]
    );
}

/**
  This function indents and prints the ACPI table Field Name.

  @param [in] Indent      Number of spaces to add to the global table indent.
                          The global table indent is 0 by default; however
                          this value is updated on entry to the ParseAcpi()
                          by adding the indent value provided to ParseAcpi()
                          and restored back on exit.
                          Therefore the total indent in the output is
                          dependent on from where this function is called.
  @param [in] FieldName   Pointer to the Field Name.
**/
VOID
EFIAPI
PrintFieldName (
  IN UINT32         Indent,
  IN CONST CHAR16*  FieldName
)
{
  Print (
    L"%*a%-*s : ",
    gIndent + Indent,
    "",
    (OUTPUT_FIELD_COLUMN_WIDTH - gIndent - Indent),
    FieldName
    );
}

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
  )
{
  ASSERT (Name != NULL);
  ASSERT (Buffer != NULL);

  return AsciiSPrint (Buffer, BufferSize, "%a [%d]", Name , Index);
}

/**
  Set all ACPI structure instance counts to 0.

  @param [in,out] StructDb     ACPI structure database with counts to reset.
**/
VOID
EFIAPI
ResetAcpiStructCounts (
  IN OUT ACPI_STRUCT_DATABASE* StructDb
  )
{
  UINT32 Type;

  ASSERT (StructDb != NULL);
  ASSERT (StructDb->Entries != NULL);

  for (Type = 0; Type < StructDb->EntryCount; Type++) {
    StructDb->Entries[Type].Count = 0;
  }
}

/**
  Sum all ACPI structure instance counts.

  @param [in] StructDb     ACPI structure database with per-type counts to sum.

  @return   Total number of structure instances recorded in the database.
**/
UINT32
EFIAPI
SumAcpiStructCounts (
  IN  CONST ACPI_STRUCT_DATABASE* StructDb
  )
{
  UINT32 Type;
  UINT32 Total;

  ASSERT (StructDb != NULL);
  ASSERT (StructDb->Entries != NULL);

  Total = 0;

  for (Type = 0; Type < StructDb->EntryCount; Type++) {
    Total += StructDb->Entries[Type].Count;
  }

  return Total;
}

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
  )
{
  UINT32 Compatible;

  ASSERT (StructDb != NULL);
  ASSERT (StructDb->Entries != NULL);

  if (Type >= StructDb->EntryCount) {
    return FALSE;
  }

#if defined (MDE_CPU_ARM) || defined (MDE_CPU_AARCH64)
  Compatible = StructDb->Entries[Type].CompatArch &
               (ARCH_COMPAT_ARM | ARCH_COMPAT_AARCH64);
#else
  Compatible = StructDb->Entries[Type].CompatArch;
#endif

  return (Compatible != 0);
}

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
  )
{
  BOOLEAN   AllValid;
  UINT32    Type;

  ASSERT (StructDb != NULL);
  ASSERT (StructDb->Entries != NULL);

  AllValid = TRUE;
  Print (L"\nTable Breakdown:\n");

  for (Type = 0; Type < StructDb->EntryCount; Type++) {
    ASSERT (Type == StructDb->Entries[Type].Type);

    if (IsAcpiStructTypeValid (Type, StructDb)) {
      Print (
        L"%*a%-*a : %d\n",
        INSTANCE_COUNT_INDENT,
        "",
        OUTPUT_FIELD_COLUMN_WIDTH - INSTANCE_COUNT_INDENT,
        StructDb->Entries[Type].Name,
        StructDb->Entries[Type].Count
        );
    } else if (StructDb->Entries[Type].Count > 0) {
      AllValid = FALSE;
      IncrementErrorCount ();
      Print (
        L"ERROR: %a Structure is not valid for the target architecture " \
          L"(found %d)\n",
        StructDb->Entries[Type].Name,
        StructDb->Entries[Type].Count
        );
    }
  }

  return AllValid;
}

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
  )
{
  ACPI_STRUCT_PARSER_FUNC ParserFunc;
  CHAR8                   Buffer[80];

  ASSERT (Ptr != NULL);
  ASSERT (StructDb != NULL);
  ASSERT (StructDb->Entries != NULL);
  ASSERT (StructDb->Name != NULL);

  PrintFieldName (Indent, L"* Offset *");
  Print (L"0x%x\n", Offset);

  if (Type >= StructDb->EntryCount) {
    IncrementErrorCount ();
    Print (L"ERROR: Unknown %a. Type = %d\n", StructDb->Name, Type);
    return FALSE;
  }

  if (StructDb->Entries[Type].Handler.ParserFunc != NULL) {
    ParserFunc = StructDb->Entries[Type].Handler.ParserFunc;
    ParserFunc (Ptr, Length, OptArg0, OptArg1);
  } else if (StructDb->Entries[Type].Handler.ParserArray != NULL) {
    ASSERT (StructDb->Entries[Type].Handler.Elements != 0);

    PrintAcpiStructName (
      StructDb->Entries[Type].Name,
      StructDb->Entries[Type].Count,
      sizeof (Buffer),
      Buffer
      );

    ParseAcpi (
      TRUE,
      Indent,
      Buffer,
      Ptr,
      Length,
      StructDb->Entries[Type].Handler.ParserArray,
      StructDb->Entries[Type].Handler.Elements
      );
  } else {
    StructDb->Entries[Type].Count++;
    Print (
      L"ERROR: Parsing of %a Structure is not implemented\n",
      StructDb->Entries[Type].Name
      );
    return FALSE;
  }

  StructDb->Entries[Type].Count++;
  return TRUE;
}

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
)
{
  UINT32  Index;
  UINT32  Offset;
  BOOLEAN HighLight;
  UINTN   OriginalAttribute;

  //
  // set local variables to suppress incorrect compiler/analyzer warnings
  //
  OriginalAttribute = 0;
  Offset = 0;

  // Increment the Indent
  gIndent += Indent;

  if (Trace && (AsciiName != NULL)){
    HighLight = GetColourHighlighting ();

    if (HighLight) {
      OriginalAttribute = gST->ConOut->Mode->Attribute;
      gST->ConOut->SetAttribute (
                     gST->ConOut,
                     EFI_TEXT_ATTR(EFI_YELLOW,
                       ((OriginalAttribute&(BIT4|BIT5|BIT6))>>4))
                     );
    }
    Print (
      L"%*a%-*a :\n",
      gIndent,
      "",
      (OUTPUT_FIELD_COLUMN_WIDTH - gIndent),
      AsciiName
      );
    if (HighLight) {
      gST->ConOut->SetAttribute (gST->ConOut, OriginalAttribute);
    }
  }

  for (Index = 0; Index < ParserItems; Index++) {
    if ((Offset + Parser[Index].Length) > Length) {

      // For fields outside the buffer length provided, reset any pointers
      // which were supposed to be updated by this function call
      if (Parser[Index].ItemPtr != NULL) {
        *Parser[Index].ItemPtr = NULL;
      }

      // We don't parse past the end of the max length specified
      continue;
    }

    if (GetConsistencyChecking () &&
        (Offset != Parser[Index].Offset)) {
      IncrementErrorCount ();
      Print (
        L"\nERROR: %a: Offset Mismatch for %s\n"
          L"CurrentOffset = %d FieldOffset = %d\n",
        AsciiName,
        Parser[Index].NameStr,
        Offset,
        Parser[Index].Offset
        );
    }

    if (Trace) {
      // if there is a Formatter function let the function handle
      // the printing else if a Format is specified in the table use
      // the Format for printing
      PrintFieldName (2, Parser[Index].NameStr);
      if (Parser[Index].PrintFormatter != NULL) {
        Parser[Index].PrintFormatter (Parser[Index].Format, Ptr);
      } else if (Parser[Index].Format != NULL) {
        switch (Parser[Index].Length) {
          case 1:
            DumpUint8 (Parser[Index].Format, Ptr);
            break;
          case 2:
            DumpUint16 (Parser[Index].Format, Ptr);
            break;
          case 4:
            DumpUint32 (Parser[Index].Format, Ptr);
            break;
          case 8:
            DumpUint64 (Parser[Index].Format, Ptr);
            break;
          default:
            Print (
              L"\nERROR: %a: CANNOT PARSE THIS FIELD, Field Length = %d\n",
              AsciiName,
              Parser[Index].Length
              );
        } // switch

        // Validating only makes sense if we are tracing
        // the parsed table entries, to report by table name.
        if (GetConsistencyChecking () &&
            (Parser[Index].FieldValidator != NULL)) {
          Parser[Index].FieldValidator (Ptr, Parser[Index].Context);
        }
      }
      Print (L"\n");
    } // if (Trace)

    if (Parser[Index].ItemPtr != NULL) {
      *Parser[Index].ItemPtr = (VOID*)Ptr;
    }

    Ptr += Parser[Index].Length;
    Offset += Parser[Index].Length;
  } // for

  // Decrement the Indent
  gIndent -= Indent;
  return Offset;
}

/**
  An array describing the ACPI Generic Address Structure.
  The GasParser array is used by the ParseAcpi function to parse and/or trace
  the GAS structure.
**/
STATIC CONST ACPI_PARSER GasParser[] = {
  {L"Address Space ID", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Register Bit Width", 1, 1, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Register Bit Offset", 1, 2, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Address Size", 1, 3, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Address", 8, 4, L"0x%lx", NULL, NULL, NULL, NULL}
};

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
  )
{
  Print (L"\n");
  return ParseAcpi (
           TRUE,
           Indent,
           NULL,
           Ptr,
           Length,
           PARSER_PARAMS (GasParser)
           );
}

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
  )
{
  DumpGasStruct (Ptr, 2, sizeof (EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE));
}

/**
  This function traces the ACPI header as described by the AcpiHeaderParser.

  @param [in] Ptr          Pointer to the start of the buffer.

  @retval Number of bytes parsed.
**/
UINT32
EFIAPI
DumpAcpiHeader (
  IN UINT8* Ptr
  )
{
  return ParseAcpi (
           TRUE,
           0,
           "ACPI Table Header",
           Ptr,
           sizeof (EFI_ACPI_DESCRIPTION_HEADER),
           PARSER_PARAMS (AcpiHeaderParser)
           );
}

/**
  This function parses the ACPI header as described by the AcpiHeaderParser.

  This function optionally returns the signature, length and revision of the
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
  )
{
  UINT32                        BytesParsed;

  BytesParsed = ParseAcpi (
                  FALSE,
                  0,
                  NULL,
                  Ptr,
                  sizeof (EFI_ACPI_DESCRIPTION_HEADER),
                  PARSER_PARAMS (AcpiHeaderParser)
                  );

  *Signature = AcpiHdrInfo.Signature;
  *Length = AcpiHdrInfo.Length;
  *Revision = AcpiHdrInfo.Revision;

  return BytesParsed;
}
