/** @file
  MADT table parser

  Copyright (c) 2016 - 2020, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.3 Specification - January 2019
    - Arm Generic Interrupt Controller Architecture Specification,
      GIC architecture version 3 and version 4, issue E
    - Arm Server Base System Architecture 5.0
**/

#include <IndustryStandard/Acpi.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"
#include "AcpiView.h"
#include "MadtParser.h"

// Local Variables
STATIC CONST UINT8* MadtInterruptControllerType;
STATIC CONST UINT8* MadtInterruptControllerLength;
STATIC ACPI_DESCRIPTION_HEADER_INFO AcpiHdrInfo;

/**
  This function validates the System Vector Base in the GICD.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateGICDSystemVectorBase (
  IN UINT8* Ptr,
  IN VOID*  Context
)
{
  if (*(UINT32*)Ptr != 0) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: System Vector Base must be zero."
    );
  }
}

/**
  This function validates the SPE Overflow Interrupt in the GICC.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateSpeOverflowInterrupt (
  IN UINT8* Ptr,
  IN VOID*  Context
  )
{
  UINT16 SpeOverflowInterrupt;

  SpeOverflowInterrupt = *(UINT16*)Ptr;

  // SPE not supported by this processor
  if (SpeOverflowInterrupt == 0) {
    return;
  }

  if ((SpeOverflowInterrupt < ARM_PPI_ID_MIN) ||
      ((SpeOverflowInterrupt > ARM_PPI_ID_MAX) &&
       (SpeOverflowInterrupt < ARM_PPI_ID_EXTENDED_MIN)) ||
      (SpeOverflowInterrupt > ARM_PPI_ID_EXTENDED_MAX)) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: SPE Overflow Interrupt ID of %d is not in the allowed PPI ID "
        L"ranges of %d-%d or %d-%d (for GICv3.1 or later).",
      SpeOverflowInterrupt,
      ARM_PPI_ID_MIN,
      ARM_PPI_ID_MAX,
      ARM_PPI_ID_EXTENDED_MIN,
      ARM_PPI_ID_EXTENDED_MAX
    );
  } else if (SpeOverflowInterrupt != ARM_PPI_ID_PMBIRQ) {
    IncrementWarningCount();
    Print (
      L"\nWARNING: SPE Overflow Interrupt ID of %d is not compliant with SBSA "
        L"Level 3 PPI ID assignment: %d.",
      SpeOverflowInterrupt,
      ARM_PPI_ID_PMBIRQ
    );
  }
}

/**
  An ACPI_PARSER array describing the GICC Interrupt Controller Structure.
**/
STATIC CONST ACPI_PARSER GicCParser[] = {
  {L"Type", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 1, 1, L"%d", NULL, NULL, NULL, NULL},
  {L"Reserved", 2, 2, L"0x%x", NULL, NULL, NULL, NULL},

  {L"CPU Interface Number", 4, 4, L"0x%x", NULL, NULL, NULL, NULL},
  {L"ACPI Processor UID", 4, 8, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Flags", 4, 12, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Parking Protocol Version", 4, 16, L"0x%x", NULL, NULL, NULL, NULL},

  {L"Performance Interrupt GSIV", 4, 20, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Parked Address", 8, 24, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Physical Base Address", 8, 32, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"GICV", 8, 40, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"GICH", 8, 48, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"VGIC Maintenance interrupt", 4, 56, L"0x%x", NULL, NULL, NULL, NULL},
  {L"GICR Base Address", 8, 60, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"MPIDR", 8, 68, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Processor Power Efficiency Class", 1, 76, L"0x%x", NULL, NULL, NULL,
   NULL},
  {L"Reserved", 1, 77, L"0x%x", NULL, NULL, NULL, NULL},
  {L"SPE overflow Interrupt", 2, 78, L"0x%x", NULL, NULL,
    ValidateSpeOverflowInterrupt, NULL}
};

/**
  An ACPI_PARSER array describing the GICD Interrupt Controller Structure.
**/
STATIC CONST ACPI_PARSER GicDParser[] = {
  {L"Type", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 1, 1, L"%d", NULL, NULL, NULL, NULL},
  {L"Reserved", 2, 2, L"0x%x", NULL, NULL, NULL, NULL},

  {L"GIC ID", 4, 4, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Physical Base Address", 8, 8, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"System Vector Base", 4, 16, L"0x%x", NULL, NULL,
    ValidateGICDSystemVectorBase, NULL},
  {L"GIC Version", 1, 20, L"%d", NULL, NULL, NULL, NULL},
  {L"Reserved", 3, 21, L"%x %x %x", Dump3Chars, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the MSI Frame Interrupt Controller Structure.
**/
STATIC CONST ACPI_PARSER GicMSIFrameParser[] = {
  {L"Type", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 1, 1, L"%d", NULL, NULL, NULL, NULL},
  {L"Reserved", 2, 2, L"0x%x", NULL, NULL, NULL, NULL},

  {L"MSI Frame ID", 4, 4, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Physical Base Address", 8, 8, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Flags", 4, 16, L"0x%x", NULL, NULL, NULL, NULL},

  {L"SPI Count", 2, 20, L"%d", NULL, NULL, NULL, NULL},
  {L"SPI Base", 2, 22, L"0x%x", NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the GICR Interrupt Controller Structure.
**/
STATIC CONST ACPI_PARSER GicRParser[] = {
  {L"Type", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 1, 1, L"%d", NULL, NULL, NULL, NULL},
  {L"Reserved", 2, 2, L"0x%x", NULL, NULL, NULL, NULL},

  {L"Discovery Range Base Address", 8, 4, L"0x%lx", NULL, NULL, NULL,
   NULL},
  {L"Discovery Range Length", 4, 12, L"0x%x", NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the GIC ITS Interrupt Controller Structure.
**/
STATIC CONST ACPI_PARSER GicITSParser[] = {
  {L"Type", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 1, 1, L"%d", NULL, NULL, NULL, NULL},
  {L"Reserved", 2, 2, L"0x%x", NULL, NULL, NULL, NULL},

  {L"GIC ITS ID", 4, 4, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Physical Base Address", 8, 8, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Reserved", 4, 16, L"0x%x", NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the ACPI MADT Table.
**/
STATIC CONST ACPI_PARSER MadtParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo),
  {L"Local Interrupt Controller Address", 4, 36, L"0x%x", NULL, NULL, NULL,
   NULL},
  {L"Flags", 4, 40, L"0x%x", NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the MADT Interrupt Controller Structure Header Structure.
**/
STATIC CONST ACPI_PARSER MadtInterruptControllerHeaderParser[] = {
  {NULL, 1, 0, NULL, NULL, (VOID**)&MadtInterruptControllerType, NULL, NULL},
  {L"Length", 1, 1, NULL, NULL, (VOID**)&MadtInterruptControllerLength, NULL,
   NULL},
  {L"Reserved", 2, 2, NULL, NULL, NULL, NULL, NULL}
};

/**
  Information about each Interrupt Controller Structure type.
**/
STATIC ACPI_STRUCT_INFO MadtStructs[] = {
  ACPI_STRUCT_INFO_PARSER_NOT_IMPLEMENTED (
    "Processor Local APIC",
    EFI_ACPI_6_3_PROCESSOR_LOCAL_APIC,
    ARCH_COMPAT_IA32 | ARCH_COMPAT_X64
    ),
  ACPI_STRUCT_INFO_PARSER_NOT_IMPLEMENTED (
    "I/O APIC",
    EFI_ACPI_6_3_IO_APIC,
    ARCH_COMPAT_IA32 | ARCH_COMPAT_X64
    ),
  ACPI_STRUCT_INFO_PARSER_NOT_IMPLEMENTED (
    "Interrupt Source Override",
    EFI_ACPI_6_3_INTERRUPT_SOURCE_OVERRIDE,
    ARCH_COMPAT_IA32 | ARCH_COMPAT_X64
    ),
  ACPI_STRUCT_INFO_PARSER_NOT_IMPLEMENTED (
    "NMI Source",
    EFI_ACPI_6_3_NON_MASKABLE_INTERRUPT_SOURCE,
    ARCH_COMPAT_IA32 | ARCH_COMPAT_X64
    ),
  ACPI_STRUCT_INFO_PARSER_NOT_IMPLEMENTED (
    "Local APIC NMI",
    EFI_ACPI_6_3_LOCAL_APIC_NMI,
    ARCH_COMPAT_IA32 | ARCH_COMPAT_X64
    ),
  ACPI_STRUCT_INFO_PARSER_NOT_IMPLEMENTED (
    "Local APIC Address Override",
    EFI_ACPI_6_3_LOCAL_APIC_ADDRESS_OVERRIDE,
    ARCH_COMPAT_IA32 | ARCH_COMPAT_X64
    ),
  ACPI_STRUCT_INFO_PARSER_NOT_IMPLEMENTED (
    "I/O SAPIC",
    EFI_ACPI_6_3_IO_SAPIC,
    ARCH_COMPAT_IA32 | ARCH_COMPAT_X64
    ),
  ACPI_STRUCT_INFO_PARSER_NOT_IMPLEMENTED (
    "Local SAPIC",
    EFI_ACPI_6_3_LOCAL_SAPIC,
    ARCH_COMPAT_IA32 | ARCH_COMPAT_X64
    ),
  ACPI_STRUCT_INFO_PARSER_NOT_IMPLEMENTED (
    "Platform Interrupt Sources",
    EFI_ACPI_6_3_PLATFORM_INTERRUPT_SOURCES,
    ARCH_COMPAT_IA32 | ARCH_COMPAT_X64
    ),
  ACPI_STRUCT_INFO_PARSER_NOT_IMPLEMENTED (
    "Processor Local x2APIC",
    EFI_ACPI_6_3_PROCESSOR_LOCAL_X2APIC,
    ARCH_COMPAT_IA32 | ARCH_COMPAT_X64
    ),
  ACPI_STRUCT_INFO_PARSER_NOT_IMPLEMENTED (
    "Local x2APIC NMI",
    EFI_ACPI_6_3_LOCAL_X2APIC_NMI,
    ARCH_COMPAT_IA32 | ARCH_COMPAT_X64
    ),
  ADD_ACPI_STRUCT_INFO_ARRAY (
    "GICC",
    EFI_ACPI_6_3_GIC,
    ARCH_COMPAT_ARM | ARCH_COMPAT_AARCH64,
    GicCParser
    ),
  ADD_ACPI_STRUCT_INFO_ARRAY (
    "GICD",
    EFI_ACPI_6_3_GICD,
    ARCH_COMPAT_ARM | ARCH_COMPAT_AARCH64,
    GicDParser
    ),
  ADD_ACPI_STRUCT_INFO_ARRAY (
    "GIC MSI Frame",
    EFI_ACPI_6_3_GIC_MSI_FRAME,
    ARCH_COMPAT_ARM | ARCH_COMPAT_AARCH64,
    GicMSIFrameParser
    ),
  ADD_ACPI_STRUCT_INFO_ARRAY (
    "GICR",
    EFI_ACPI_6_3_GICR,
    ARCH_COMPAT_ARM | ARCH_COMPAT_AARCH64,
    GicRParser
    ),
  ADD_ACPI_STRUCT_INFO_ARRAY (
    "GIC ITS",
    EFI_ACPI_6_3_GIC_ITS,
    ARCH_COMPAT_ARM | ARCH_COMPAT_AARCH64,
    GicITSParser
    )
};

/**
  MADT structure database
**/
STATIC ACPI_STRUCT_DATABASE MadtDatabase = {
  "Interrupt Controller Structure",
  MadtStructs,
  ARRAY_SIZE (MadtStructs)
};

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
  )
{
  UINT32 Offset;
  UINT8* InterruptContollerPtr;

  if (!Trace) {
    return;
  }

  ResetAcpiStructCounts (&MadtDatabase);

  Offset = ParseAcpi (
             TRUE,
             0,
             "MADT",
             Ptr,
             AcpiTableLength,
             PARSER_PARAMS (MadtParser)
             );
  InterruptContollerPtr = Ptr + Offset;

  while (Offset < AcpiTableLength) {
    // Parse Interrupt Controller Structure to obtain Length.
    ParseAcpi (
      FALSE,
      0,
      NULL,
      InterruptContollerPtr,
      AcpiTableLength - Offset,
      PARSER_PARAMS (MadtInterruptControllerHeaderParser)
      );

    // Check if the values used to control the parsing logic have been
    // successfully read.
    if ((MadtInterruptControllerType == NULL) ||
        (MadtInterruptControllerLength == NULL)) {
      IncrementErrorCount ();
      Print (
        L"ERROR: Insufficient remaining table buffer length to read the " \
          L"%a header. Length = %d.\n",
        MadtDatabase.Name,
        AcpiTableLength - Offset
        );
      return;
    }

    // Validate Interrupt Controller Structure length
    if ((*MadtInterruptControllerLength == 0) ||
        ((Offset + (*MadtInterruptControllerLength)) > AcpiTableLength)) {
      IncrementErrorCount ();
      Print (
        L"ERROR: Invalid %a length. Length = %d. Offset = %d. " \
          "AcpiTableLength = %d.\n",
        MadtDatabase.Name,
        *MadtInterruptControllerLength,
        Offset,
        AcpiTableLength
        );
      return;
    }

    // Parse the Interrupt Controller Structure
    ParseAcpiStruct (
      2,
      InterruptContollerPtr,
      &MadtDatabase,
      Offset,
      *MadtInterruptControllerType,
      *MadtInterruptControllerLength,
      NULL,
      NULL
      );

    InterruptContollerPtr += *MadtInterruptControllerLength;
    Offset += *MadtInterruptControllerLength;
  } // while

  // Report and validate Interrupt Controller Structure counts
  if (GetConsistencyChecking ()) {
    ValidateAcpiStructCounts (&MadtDatabase);

    if (MadtStructs[EFI_ACPI_6_3_GICD].Count > 1) {
      IncrementErrorCount ();
      Print (
        L"ERROR: Only one %a must be present\n",
        MadtStructs[EFI_ACPI_6_3_GICD].Name
        );
    }
  }
}
