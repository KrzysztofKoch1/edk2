/** @file
  GTDT table parser

  Copyright (c) 2016 - 2020, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.3 Specification - January 2019
  **/

#include <IndustryStandard/Acpi.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"
#include "AcpiView.h"

// "The number of GT Block Timers must be less than or equal to 8"
#define GT_BLOCK_TIMER_COUNT_MAX 8

/**
  Handler for each Platform Timer Structure type
**/
STATIC ACPI_STRUCT_INFO GtdtStructs[];

// Local variables
STATIC CONST UINT32* GtdtPlatformTimerCount;
STATIC CONST UINT32* GtdtPlatformTimerOffset;
STATIC CONST UINT8*  PlatformTimerType;
STATIC CONST UINT16* PlatformTimerLength;
STATIC CONST UINT32* GtBlockTimerCount;
STATIC CONST UINT32* GtBlockTimerOffset;
STATIC ACPI_DESCRIPTION_HEADER_INFO AcpiHdrInfo;

/**
  This function validates the GT Block timer count.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateGtBlockTimerCount (
  IN UINT8* Ptr,
  IN VOID*  Context
  )
{
  UINT32 BlockTimerCount;

  BlockTimerCount = *(UINT32*)Ptr;

  if (BlockTimerCount > GT_BLOCK_TIMER_COUNT_MAX) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: Timer Count = %d. Max Timer Count is %d.",
      BlockTimerCount,
      GT_BLOCK_TIMER_COUNT_MAX
      );
  }
}

/**
  This function validates the GT Frame Number.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateGtFrameNumber (
  IN UINT8* Ptr,
  IN VOID*  Context
  )
{
  UINT8 FrameNumber;

  FrameNumber = *(UINT8*)Ptr;

  if (FrameNumber >= GT_BLOCK_TIMER_COUNT_MAX) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: GT Frame Number = %d. GT Frame Number must be in range 0-%d.",
      FrameNumber,
      GT_BLOCK_TIMER_COUNT_MAX - 1
      );
  }
}

/**
  An ACPI_PARSER array describing the ACPI GTDT Table.
**/
STATIC CONST ACPI_PARSER GtdtParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo),
  {L"CntControlBase Physical Address", 8, 36, L"0x%lx", NULL, NULL,
   NULL, NULL},
  {L"Reserved", 4, 44, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Secure EL1 timer GSIV", 4, 48, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Secure EL1 timer FLAGS", 4, 52, L"0x%x", NULL, NULL, NULL, NULL},

  {L"Non-Secure EL1 timer GSIV", 4, 56, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Non-Secure EL1 timer FLAGS", 4, 60, L"0x%x", NULL, NULL, NULL, NULL},

  {L"Virtual timer GSIV", 4, 64, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Virtual timer FLAGS", 4, 68, L"0x%x", NULL, NULL, NULL, NULL},

  {L"Non-Secure EL2 timer GSIV", 4, 72, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Non-Secure EL2 timer FLAGS", 4, 76, L"0x%x", NULL, NULL, NULL, NULL},

  {L"CntReadBase Physical address", 8, 80, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Platform Timer Count", 4, 88, L"%d", NULL,
   (VOID**)&GtdtPlatformTimerCount, NULL, NULL},
  {L"Platform Timer Offset", 4, 92, L"0x%x", NULL,
   (VOID**)&GtdtPlatformTimerOffset, NULL, NULL},
  {L"Virtual EL2 Timer GSIV", 4, 96, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Virtual EL2 Timer Flags", 4, 100, L"0x%x", NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the Platform timer header.
**/
STATIC CONST ACPI_PARSER GtPlatformTimerHeaderParser[] = {
  {L"Type", 1, 0, NULL, NULL, (VOID**)&PlatformTimerType, NULL, NULL},
  {L"Length", 2, 1, NULL, NULL, (VOID**)&PlatformTimerLength, NULL, NULL},
  {L"Reserved", 1, 3, NULL, NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the Platform GT Block.
**/
STATIC CONST ACPI_PARSER GtBlockParser[] = {
  {L"Type", 1, 0, L"%d", NULL, NULL, NULL, NULL},
  {L"Length", 2, 1, L"%d", NULL, NULL, NULL, NULL},
  {L"Reserved", 1, 3, L"%x", NULL, NULL, NULL, NULL},
  {L"Physical address (CntCtlBase)", 8, 4, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Timer Count", 4, 12, L"%d", NULL, (VOID**)&GtBlockTimerCount,
   ValidateGtBlockTimerCount, NULL},
  {L"Timer Offset", 4, 16, L"%d", NULL, (VOID**)&GtBlockTimerOffset, NULL,
    NULL}
};

/**
  An ACPI_PARSER array describing the GT Block timer.
**/
STATIC CONST ACPI_PARSER GtBlockTimerParser[] = {
  {L"Frame Number", 1, 0, L"%d", NULL, NULL, ValidateGtFrameNumber, NULL},
  {L"Reserved", 3, 1, L"%x %x %x", Dump3Chars, NULL, NULL, NULL},
  {L"Physical address (CntBaseX)", 8, 4, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Physical address (CntEL0BaseX)", 8, 12, L"0x%lx", NULL, NULL, NULL,
    NULL},
  {L"Physical Timer GSIV", 4, 20, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Physical Timer Flags", 4, 24, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Virtual Timer GSIV", 4, 28, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Virtual Timer Flags", 4, 32, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Common Flags", 4, 36, L"0x%x", NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the Platform Watchdog.
**/
STATIC CONST ACPI_PARSER SBSAGenericWatchdogParser[] = {
  {L"Type", 1, 0, L"%d", NULL, NULL, NULL, NULL},
  {L"Length", 2, 1, L"%d", NULL, NULL, NULL, NULL},
  {L"Reserved", 1, 3, L"%x", NULL, NULL, NULL, NULL},
  {L"RefreshFrame Physical address", 8, 4, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"ControlFrame Physical address", 8, 12, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Watchdog Timer GSIV", 4, 20, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Watchdog Timer Flags", 4, 24, L"0x%x", NULL, NULL, NULL, NULL}
};

/**
  This function parses the Platform GT Block.

  @param [in] Ptr         Pointer to the start of structure's buffer.
  @param [in] Length      Length of the buffer.
  @param [in] OptArg0     First optional argument (Not used).
  @param [in] OptArg1     Second optional argument (Not used).
**/
STATIC
VOID
DumpGTBlock (
  IN       UINT8* Ptr,
  IN       UINT32 Length,
  IN CONST VOID*  OptArg0 OPTIONAL,
  IN CONST VOID*  OptArg1 OPTIONAL
  )
{
  UINT32 Index;
  UINT32 Offset;
  CHAR8  Buffer[80];

  PrintAcpiStructName (
    GtdtStructs[EFI_ACPI_6_3_GTDT_GT_BLOCK].Name,
    GtdtStructs[EFI_ACPI_6_3_GTDT_GT_BLOCK].Count,
    sizeof (Buffer),
    Buffer
    );

  ParseAcpi (
    TRUE,
    2,
    Buffer,
    Ptr,
    Length,
    PARSER_PARAMS (GtBlockParser)
    );

  // Check if the values used to control the parsing logic have been
  // successfully read.
  if ((GtBlockTimerCount == NULL) ||
      (GtBlockTimerOffset == NULL)) {
    IncrementErrorCount ();
    Print (
      L"ERROR: Insufficient %a Structure length. Length = %d.\n",
      GtdtStructs[EFI_ACPI_6_3_GTDT_GT_BLOCK].Name,
      Length
      );
    return;
  }

  Offset = *GtBlockTimerOffset;
  Index = 0;

  // Parse the specified number of GT Block Timer Structures or the GT Block
  // Structure buffer length. Whichever is minimum.
  while ((Index < *GtBlockTimerCount) &&
         (Offset < Length)) {
    PrintAcpiStructName ("GT Block Timer", Index, sizeof (Buffer), Buffer);
    Offset += ParseAcpi (
                TRUE,
                4,
                Buffer,
                Ptr + Offset,
                Length - Offset,
                PARSER_PARAMS (GtBlockTimerParser)
                );
    Index++;
  }
}

/**
  Information about each Platform Timer Structure type.
**/
STATIC ACPI_STRUCT_INFO GtdtStructs[] = {
  ADD_ACPI_STRUCT_INFO_FUNC (
    "GT Block",
    EFI_ACPI_6_3_GTDT_GT_BLOCK,
    ARCH_COMPAT_ARM | ARCH_COMPAT_AARCH64,
    DumpGTBlock
    ),
  ADD_ACPI_STRUCT_INFO_ARRAY (
    "SBSA Generic Watchdog",
    EFI_ACPI_6_3_GTDT_SBSA_GENERIC_WATCHDOG,
    ARCH_COMPAT_ARM | ARCH_COMPAT_AARCH64,
    SBSAGenericWatchdogParser
    )
};

/**
  GTDT structure database
**/
STATIC ACPI_STRUCT_DATABASE GtdtDatabase = {
  "Platform Timer Structure",
  GtdtStructs,
  ARRAY_SIZE (GtdtStructs)
};

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
  )
{
  UINT32 Index;
  UINT32 Offset;
  UINT8* TimerPtr;

  if (!Trace) {
    return;
  }

  ResetAcpiStructCounts (&GtdtDatabase);

  ParseAcpi (
    TRUE,
    0,
    "GTDT",
    Ptr,
    AcpiTableLength,
    PARSER_PARAMS (GtdtParser)
    );

  // Check if the values used to control the parsing logic have been
  // successfully read.
  if ((GtdtPlatformTimerCount == NULL) ||
      (GtdtPlatformTimerOffset == NULL)) {
    IncrementErrorCount ();
    Print (
      L"ERROR: Insufficient table length. AcpiTableLength = %d.\n",
      AcpiTableLength
      );
    return;
  }

  TimerPtr = Ptr + *GtdtPlatformTimerOffset;
  Offset = *GtdtPlatformTimerOffset;
  Index = 0;

  // Parse the specified number of Platform Timer Structures or the GTDT
  // buffer length. Whichever is minimum.
  while ((Index++ < *GtdtPlatformTimerCount) &&
         (Offset < AcpiTableLength)) {
    // Parse the Platform Timer Header to obtain Length and Type
    ParseAcpi (
      FALSE,
      0,
      NULL,
      TimerPtr,
      AcpiTableLength - Offset,
      PARSER_PARAMS (GtPlatformTimerHeaderParser)
      );

    // Check if the values used to control the parsing logic have been
    // successfully read.
    if ((PlatformTimerType == NULL) ||
        (PlatformTimerLength == NULL)) {
      IncrementErrorCount ();
      Print (
        L"ERROR: Insufficient remaining table buffer length to read the " \
          L"%a header. Length = %d.\n",
        GtdtDatabase.Name,
        AcpiTableLength - Offset
        );
      return;
    }

    // Validate Platform Timer Structure length
    if ((*PlatformTimerLength == 0) ||
        ((Offset + (*PlatformTimerLength)) > AcpiTableLength)) {
      IncrementErrorCount ();
      Print (
        L"ERROR: Invalid %a length. Length = %d. Offset = %d. " \
          L"AcpiTableLength = %d.\n",
        GtdtDatabase.Name,
        *PlatformTimerLength,
        Offset,
        AcpiTableLength
        );
      return;
    }

    // Parse the Platform Timer Structure
    ParseAcpiStruct (
      2,
      TimerPtr,
      &GtdtDatabase,
      Offset,
      *PlatformTimerType,
      *PlatformTimerLength,
      NULL,
      NULL
      );

    TimerPtr += *PlatformTimerLength;
    Offset += *PlatformTimerLength;
  } // while

  // Report and validate Platform Timer Type Structure counts
  if (GetConsistencyChecking ()) {
    ValidateAcpiStructCounts (&GtdtDatabase);
  }
}
