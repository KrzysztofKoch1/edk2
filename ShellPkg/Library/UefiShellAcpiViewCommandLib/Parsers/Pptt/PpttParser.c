/** @file
  PPTT table parser

  Copyright (c) 2019 - 2020, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.3 Specification - January 2019
    - ARM Architecture Reference Manual ARMv8 (D.a)
**/

#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiView.h"
#include "PpttParser.h"

// Local variables
STATIC CONST UINT8*  ProcessorTopologyStructureType;
STATIC CONST UINT8*  ProcessorTopologyStructureLength;
STATIC CONST UINT32* NumberOfPrivateResources;
STATIC ACPI_DESCRIPTION_HEADER_INFO AcpiHdrInfo;

/**
  Handler for each Processor Topology Structure
**/
STATIC ACPI_STRUCT_INFO PpttStructs[];

/**
  This function validates the Cache Type Structure (Type 1) 'Number of sets'
  field.

  @param [in] Ptr       Pointer to the start of the field data.
  @param [in] Context   Pointer to context specific information e.g. this
                        could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateCacheNumberOfSets (
  IN UINT8* Ptr,
  IN VOID*  Context
  )
{
  UINT32 NumberOfSets;
  NumberOfSets = *(UINT32*)Ptr;

  if (NumberOfSets == 0) {
    IncrementErrorCount ();
    Print (L"\nERROR: Cache number of sets must be greater than 0");
    return;
  }

#if defined(MDE_CPU_ARM) || defined (MDE_CPU_AARCH64)
  if (NumberOfSets > PPTT_ARM_CCIDX_CACHE_NUMBER_OF_SETS_MAX) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: When ARMv8.3-CCIDX is implemented the maximum cache number of "
        L"sets must be less than or equal to %d",
      PPTT_ARM_CCIDX_CACHE_NUMBER_OF_SETS_MAX
      );
    return;
  }

  if (NumberOfSets > PPTT_ARM_CACHE_NUMBER_OF_SETS_MAX) {
    IncrementWarningCount ();
    Print (
      L"\nWARNING: Without ARMv8.3-CCIDX, the maximum cache number of sets "
        L"must be less than or equal to %d. Ignore this message if "
        L"ARMv8.3-CCIDX is implemented",
      PPTT_ARM_CACHE_NUMBER_OF_SETS_MAX
      );
    return;
  }
#endif

}

/**
  This function validates the Cache Type Structure (Type 1) 'Associativity'
  field.

  @param [in] Ptr       Pointer to the start of the field data.
  @param [in] Context   Pointer to context specific information e.g. this
                        could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateCacheAssociativity (
  IN UINT8* Ptr,
  IN VOID*  Context
  )
{
  UINT8 Associativity;
  Associativity = *(UINT8*)Ptr;

  if (Associativity == 0) {
    IncrementErrorCount ();
    Print (L"\nERROR: Cache associativity must be greater than 0");
    return;
  }
}

/**
  This function validates the Cache Type Structure (Type 1) Line size field.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateCacheLineSize (
  IN UINT8* Ptr,
  IN VOID*  Context
  )
{
#if defined(MDE_CPU_ARM) || defined (MDE_CPU_AARCH64)
  // Reference: ARM Architecture Reference Manual ARMv8 (D.a)
  // Section D12.2.25: CCSIDR_EL1, Current Cache Size ID Register
  //   LineSize, bits [2:0]
  //     (Log2(Number of bytes in cache line)) - 4.

  UINT16 LineSize;
  LineSize = *(UINT16*)Ptr;

  if ((LineSize < PPTT_ARM_CACHE_LINE_SIZE_MIN) ||
      (LineSize > PPTT_ARM_CACHE_LINE_SIZE_MAX)) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: The cache line size must be between %d and %d bytes"
        L" on ARM Platforms.",
      PPTT_ARM_CACHE_LINE_SIZE_MIN,
      PPTT_ARM_CACHE_LINE_SIZE_MAX
      );
    return;
  }

  if ((LineSize & (LineSize - 1)) != 0) {
    IncrementErrorCount ();
    Print (L"\nERROR: The cache line size is not a power of 2.");
  }
#endif
}

/**
  This function validates the Cache Type Structure (Type 1) Attributes field.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateCacheAttributes (
  IN UINT8* Ptr,
  IN VOID*  Context
  )
{
  // Reference: Advanced Configuration and Power Interface (ACPI) Specification
  //            Version 6.2 Errata A, September 2017
  // Table 5-153: Cache Type Structure
  UINT8 Attributes;
  Attributes = *(UINT8*)Ptr;

  if ((Attributes & 0xE0) != 0) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: Attributes bits [7:5] are reserved and must be zero.",
      Attributes
      );
    return;
  }
}

/**
  An ACPI_PARSER array describing the ACPI PPTT Table.
**/
STATIC CONST ACPI_PARSER PpttParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo)
};

/**
  An ACPI_PARSER array describing the processor topology structure header.
**/
STATIC CONST ACPI_PARSER ProcessorTopologyStructureHeaderParser[] = {
  {L"Type", 1, 0, NULL, NULL, (VOID**)&ProcessorTopologyStructureType,
   NULL, NULL},
  {L"Length", 1, 1, NULL, NULL, (VOID**)&ProcessorTopologyStructureLength,
   NULL, NULL},
  {L"Reserved", 2, 2, NULL, NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the Processor Hierarchy Node Structure - Type 0.
**/
STATIC CONST ACPI_PARSER ProcessorHierarchyNodeStructureParser[] = {
  {L"Type", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 1, 1, L"%d", NULL, NULL, NULL, NULL},
  {L"Reserved", 2, 2, L"0x%x", NULL, NULL, NULL, NULL},

  {L"Flags", 4, 4, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Parent", 4, 8, L"0x%x", NULL, NULL, NULL, NULL},
  {L"ACPI Processor ID", 4, 12, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Number of private resources", 4, 16, L"%d", NULL,
   (VOID**)&NumberOfPrivateResources, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the Cache Type Structure - Type 1.
**/
STATIC CONST ACPI_PARSER CacheTypeStructureParser[] = {
  {L"Type", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 1, 1, L"%d", NULL, NULL, NULL, NULL},
  {L"Reserved", 2, 2, L"0x%x", NULL, NULL, NULL, NULL},

  {L"Flags", 4, 4, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Next Level of Cache", 4, 8, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Size", 4, 12, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Number of sets", 4, 16, L"%d", NULL, NULL, ValidateCacheNumberOfSets, NULL},
  {L"Associativity", 1, 20, L"%d", NULL, NULL, ValidateCacheAssociativity, NULL},
  {L"Attributes", 1, 21, L"0x%x", NULL, NULL, ValidateCacheAttributes, NULL},
  {L"Line size", 2, 22, L"%d", NULL, NULL, ValidateCacheLineSize, NULL}
};

/**
  An ACPI_PARSER array describing the ID Type Structure - Type 2.
**/
STATIC CONST ACPI_PARSER IdStructureParser[] = {
  {L"Type", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 1, 1, L"%d", NULL, NULL, NULL, NULL},
  {L"Reserved", 2, 2, L"0x%x", NULL, NULL, NULL, NULL},

  {L"VENDOR_ID", 4, 4, NULL, Dump4Chars, NULL, NULL, NULL},
  {L"LEVEL_1_ID", 8, 8, L"0x%x", NULL, NULL, NULL, NULL},
  {L"LEVEL_2_ID", 8, 16, L"0x%x", NULL, NULL, NULL, NULL},
  {L"MAJOR_REV", 2, 24, L"0x%x", NULL, NULL, NULL, NULL},
  {L"MINOR_REV", 2, 26, L"0x%x", NULL, NULL, NULL, NULL},
  {L"SPIN_REV", 2, 28, L"0x%x", NULL, NULL, NULL, NULL},
};

/**
  This function parses the Processor Hierarchy Node Structure (Type 0).

  @param [in] Ptr     Pointer to the start of the Processor Hierarchy Node
                      Structure data.
  @param [in] Length  Length of the Processor Hierarchy Node Structure.
  @param [in] OptArg0 First optional argument (Not used).
  @param [in] OptArg1 Second optional argument (Not used).
**/
STATIC
VOID
DumpProcessorHierarchyNodeStructure (
  IN       UINT8* Ptr,
  IN       UINT32 Length,
  IN CONST VOID*  OptArg0 OPTIONAL,
  IN CONST VOID*  OptArg1 OPTIONAL
  )
{
  UINT32 Offset;
  UINT32 Index;
  CHAR16 Buffer[OUTPUT_FIELD_COLUMN_WIDTH];
  CHAR8  AsciiBuffer[80];

  PrintAcpiStructName (
    PpttStructs[EFI_ACPI_6_3_PPTT_TYPE_PROCESSOR].Name,
    PpttStructs[EFI_ACPI_6_3_PPTT_TYPE_PROCESSOR].Count,
    sizeof (AsciiBuffer),
    AsciiBuffer
    );

  Offset = ParseAcpi (
             TRUE,
             2,
             AsciiBuffer,
             Ptr,
             Length,
             PARSER_PARAMS (ProcessorHierarchyNodeStructureParser)
             );

  // Check if the values used to control the parsing logic have been
  // successfully read.
  if (NumberOfPrivateResources == NULL) {
    IncrementErrorCount ();
    Print (
      L"ERROR: Insufficient %a Structure length. Length = %d.\n",
      PpttStructs[EFI_ACPI_6_3_PPTT_TYPE_PROCESSOR].Name,
      Length
      );
    return;
  }

  // Make sure the Private Resource array lies inside this structure
  if (Offset + (*NumberOfPrivateResources * sizeof (UINT32)) > Length) {
    IncrementErrorCount ();
    Print (
      L"ERROR: Invalid Number of Private Resources. " \
        L"PrivateResourceCount = %d. RemainingBufferLength = %d. " \
        L"Parsing of this structure aborted.\n",
      *NumberOfPrivateResources,
      Length - Offset
      );
    return;
  }

  Index = 0;

  // Parse the specified number of private resource references or the Processor
  // Hierarchy Node length. Whichever is minimum.
  while (Index < *NumberOfPrivateResources) {
    UnicodeSPrint (
      Buffer,
      sizeof (Buffer),
      L"Private resource [%d]",
      Index
      );

    PrintFieldName (4, Buffer);
    Print (
      L"0x%x\n",
      *((UINT32*)(Ptr + Offset))
      );

    Offset += sizeof (UINT32);
    Index++;
  }
}

/**
  Information about each Processor Topology Structure type.
**/
STATIC ACPI_STRUCT_INFO PpttStructs[] = {
  ADD_ACPI_STRUCT_INFO_FUNC (
    "Processor",
    EFI_ACPI_6_3_PPTT_TYPE_PROCESSOR,
    ARCH_COMPAT_IA32 | ARCH_COMPAT_X64 | ARCH_COMPAT_ARM | ARCH_COMPAT_AARCH64,
    DumpProcessorHierarchyNodeStructure
    ),
  ADD_ACPI_STRUCT_INFO_ARRAY (
    "Cache",
    EFI_ACPI_6_3_PPTT_TYPE_CACHE,
    ARCH_COMPAT_IA32 | ARCH_COMPAT_X64 | ARCH_COMPAT_ARM | ARCH_COMPAT_AARCH64,
    CacheTypeStructureParser
    ),
  ADD_ACPI_STRUCT_INFO_ARRAY (
    "ID",
    EFI_ACPI_6_3_PPTT_TYPE_ID,
    ARCH_COMPAT_IA32 | ARCH_COMPAT_X64 | ARCH_COMPAT_ARM | ARCH_COMPAT_AARCH64,
    IdStructureParser
    )
};

/**
  PPTT structure database
**/
STATIC ACPI_STRUCT_DATABASE PpttDatabase = {
  "Processor Topology Structure",
  PpttStructs,
  ARRAY_SIZE (PpttStructs)
};

/**
  This function parses the ACPI PPTT table.
  When trace is enabled this function parses the PPTT table and
  traces the ACPI table fields.

  This function parses the following processor topology structures:
    - Processor hierarchy node structure (Type 0)
    - Cache Type Structure (Type 1)
    - ID structure (Type 2)

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
  )
{
  UINT32 Offset;
  UINT8* ProcessorTopologyStructurePtr;

  if (!Trace) {
    return;
  }

  ResetAcpiStructCounts (&PpttDatabase);

  Offset = ParseAcpi (
             TRUE,
             0,
             "PPTT",
             Ptr,
             AcpiTableLength,
             PARSER_PARAMS (PpttParser)
             );

  ProcessorTopologyStructurePtr = Ptr + Offset;

  while (Offset < AcpiTableLength) {
    // Parse Processor Hierarchy Node Structure to obtain Type and Length.
    ParseAcpi (
      FALSE,
      0,
      NULL,
      ProcessorTopologyStructurePtr,
      AcpiTableLength - Offset,
      PARSER_PARAMS (ProcessorTopologyStructureHeaderParser)
      );

    // Check if the values used to control the parsing logic have been
    // successfully read.
    if ((ProcessorTopologyStructureType == NULL) ||
        (ProcessorTopologyStructureLength == NULL)) {
      IncrementErrorCount ();
      Print (
        L"ERROR: Insufficient remaining table buffer length to read the " \
          L"%a header. Length = %d.\n",
        PpttDatabase.Name,
        AcpiTableLength - Offset
        );
      return;
    }

    // Validate Processor Topology Structure length
    if ((*ProcessorTopologyStructureLength == 0) ||
        ((Offset + (*ProcessorTopologyStructureLength)) > AcpiTableLength)) {
      IncrementErrorCount ();
      Print (
        L"ERROR: Invalid %a length. Length = %d. Offset = %d. " \
          L"AcpiTableLength = %d.\n",
        PpttDatabase.Name,
        *ProcessorTopologyStructureLength,
        Offset,
        AcpiTableLength
        );
      return;
    }

    // Parse the Processor Topology Structure
    ParseAcpiStruct (
      2,
      ProcessorTopologyStructurePtr,
      &PpttDatabase,
      Offset,
      *ProcessorTopologyStructureType,
      *ProcessorTopologyStructureLength,
      NULL,
      NULL
      );

    ProcessorTopologyStructurePtr += *ProcessorTopologyStructureLength;
    Offset += *ProcessorTopologyStructureLength;
  } // while

  // Report and validate processor topology structure counts
  if (GetConsistencyChecking ()) {
    ValidateAcpiStructCounts (&PpttDatabase);
  }
}
