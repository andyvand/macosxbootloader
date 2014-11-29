#include <mach-o/fat.h>
#include <mach/machine.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

uint32_t __OSSwapInt32(uint32_t x);

#if defined(__ppc__) || defined(__ppc64__)
#define ARCHSWAP(x) (x)
#define PPCSWAP(x) __OSSwapInt32(x)
#else
#define ARCHSWAP(x) __OSSwapInt32(x)
#define PPCSWAP(x) x
#endif

typedef struct filehdr
{
	uint16_t f_magic;		/* Magic number.		*/
	uint16_t f_nscns;		/* Number of sections.		*/
	uint32_t f_timdat;		/* Time & date stamp.		*/
	uint32_t f_symptr;		/* File pointer to symtab.	*/
	uint32_t f_nsyms;		/* Number of symtab entries.	*/
	uint16_t f_opthdr;		/* Sizeof(optional hdr).	*/
	uint16_t f_flags;		/* Flags.			*/
} filehdr_t;

typedef struct aouthdr
{
	uint16_t magic;		/* type of file				*/
	uint16_t vstamp;	/* version stamp			*/
	uint32_t tsize;		/* text size in bytes, padded to FW bdry*/
	uint32_t dsize;		/* initialized data "  "		*/
	uint32_t bsize;		/* uninitialized data "   "		*/
	uint32_t entry;		/* entry pt.				*/
	uint32_t text_start;	/* base of text used for this file 	*/
	uint32_t data_start;	/* base of data used for this file 	*/

	uint32_t ImageBase;
	uint32_t SectionAlignment;
	uint32_t FileAlignment;
	uint16_t MajorOperatingSystemVersion;
	uint16_t MinorOperatingSystemVersion;
	uint16_t MajorImageVersion;
	uint16_t MinorImageVersion;
	uint16_t MajorSubsystemVersion;
	uint16_t MinorSubsystemVersion;
	uint32_t Win32VersionValue;	/* Reserved, must be zero. */
	uint32_t SizeOfImage;
	uint32_t SizeOfHeaders;
	uint32_t CheckSum;
	uint16_t Subsystem;
	uint16_t DllCharacteristics;
	uint32_t SizeOfStackReserve;
	uint32_t SizeOfStackCommit;
	uint32_t SizeOfHeapReserve;
	uint32_t SizeOfHeapCommit;
	uint32_t LoaderFlags;
	uint32_t NumberOfRvaAndSizes;
	uint32_t DataDirectory[16][2]; /* 16 entries, 2 elements/entry, */
} aouthdr_t;

typedef struct aouthdr_64
{
	uint16_t magic;		/* type of file				*/
	uint16_t vstamp;	/* version stamp			*/
	uint32_t tsize;		/* text size in bytes, padded to FW bdry*/
	uint32_t dsize;		/* initialized data "  "		*/
	uint32_t bsize;		/* uninitialized data "   "		*/
	uint32_t entry;		/* entry pt.				*/
	uint32_t text_start;	/* base of text used for this file 	*/
	/* note no base of data field in the 64-bit optional header */

	uint64_t ImageBase;
	uint32_t SectionAlignment;
	uint32_t FileAlignment;
	uint16_t MajorOperatingSystemVersion;
	uint16_t MinorOperatingSystemVersion;
	uint16_t MajorImageVersion;
	uint16_t MinorImageVersion;
	uint16_t MajorSubsystemVersion;
	uint16_t MinorSubsystemVersion;
	uint32_t Win32VersionValue;	/* Reserved, must be zero. */
	uint32_t SizeOfImage;
	uint32_t SizeOfHeaders;
	uint32_t CheckSum;
	uint16_t Subsystem;
	uint16_t DllCharacteristics;
	uint64_t SizeOfStackReserve;
	uint64_t SizeOfStackCommit;
	uint64_t SizeOfHeapReserve;
	uint64_t SizeOfHeapCommit;
	uint32_t LoaderFlags;
	uint32_t NumberOfRvaAndSizes;
	uint32_t DataDirectory[16][2]; /* 16 entries, 2 elements/entry, */
} aouthdr_64_t;

typedef struct tehdr
{
	uint16_t Signature;        // signature for TE format = "VZ"
	uint16_t Machine;          // from the original file header
	uint8_t NumberOfSections;  // from the original file header
	uint8_t Subsystem;         // from original optional header
	uint16_t StrippedSize;     // how many bytes we removed from the header
	uint32_t AddressOfEntryPoint; // offset to entry point -- from original optional header
	uint32_t BaseOfCode;       // from original image -- required for ITP debug
	uint64_t ImageBase;        // from original file header
	uint32_t DataDirectory[16][2]; // only base relocation and debug directory
} tehdr_t;

typedef struct ms_dos_stub
{
  /* DOS header fields - always at offset zero in the EXE file.  */
  uint16_t e_magic;	/* Magic number, 0x5a4d. (DOSMAGIC above) */
  uint16_t e_cblp;	/* Bytes on last page of file, 0x90.  */
  uint16_t e_cp;	/* Pages in file, 0x3.  */
  uint16_t e_crlc;	/* Relocations, 0x0.  */
  uint16_t e_cparhdr;	/* Size of header in paragraphs, 0x4.  */
  uint16_t e_minalloc;	/* Minimum extra paragraphs needed, 0x0.  */
  uint16_t e_maxalloc;	/* Maximum extra paragraphs needed, 0xFFFF.  */
  uint16_t e_ss;	/* Initial (relative) SS value, 0x0.  */
  uint16_t e_sp;	/* Initial SP value, 0xb8.  */
  uint16_t e_csum;	/* Checksum, 0x0.  */
  uint16_t e_ip;	/* Initial IP value, 0x0.  */
  uint16_t e_cs;	/* Initial (relative) CS value, 0x0.  */
  uint16_t e_lfarlc;	/* File address of relocation table, 0x40.  */
  uint16_t e_ovno;	/* Overlay number, 0x0.  */
  uint16_t e_res[4];	/* Reserved words, all 0x0.  */
  uint16_t e_oemid;	/* OEM identifier (for e_oeminfo), 0x0.  */
  uint16_t e_oeminfo;	/* OEM information; e_oemid specific, 0x0.  */
  uint16_t e_res2[10];	/* Reserved words, all 0x0.  */
  uint32_t e_lfanew;	/* File address of new exe header, usually 0x80.  */
} ms_dos_stub_t;

#define IMAGE_FILE_MACHINE_UNKNOWN           0
#define IMAGE_FILE_MACHINE_ARM               0x01c0
#define IMAGE_FILE_MACHINE_THUMB             0x01c2
#define IMAGE_FILE_MACHINE_ARMV7             0x01c4
#define IMAGE_FILE_MACHINE_ARM64             0x01c6
#define IMAGE_FILE_MACHINE_POWERPC           0x01F0
#define IMAGE_FILE_MACHINE_POWERPCFP         0x01f1
#define IMAGE_FILE_MACHINE_I386              0x014c
#define IMAGE_FILE_MACHINE_R3000             0x0162
#define IMAGE_FILE_MACHINE_R4000             0x0166
#define IMAGE_FILE_MACHINE_R10000            0x0168
#define IMAGE_FILE_MACHINE_WCEMIPSV2         0x0169
#define IMAGE_FILE_MACHINE_ALPHA             0x0184
#define IMAGE_FILE_MACHINE_IA64              0x0200
#define IMAGE_FILE_MACHINE_MIPS16            0x0266
#define IMAGE_FILE_MACHINE_ALPHA64           0x0284
#define IMAGE_FILE_MACHINE_MIPSFPU           0x0366
#define IMAGE_FILE_MACHINE_MIPSFPU16         0x0466
#define IMAGE_FILE_MACHINE_CEE               0x0CEE
#define IMAGE_FILE_MACHINE_CEF               0x0CEF
#define IMAGE_FILE_MACHINE_EBC               0x0EBC
#define IMAGE_FILE_MACHINE_SPARC             0x2000
#define IMAGE_FILE_MACHINE_AMD64             0x8664

#if defined(__ppc__) && defined(__ppc64__)
#define EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC 0xb01
#define EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC 0xb02

#define EFI_IMAGE_DOS_SIGNATURE     0x4D5A  // ZM
#define EFI_IMAGE_NT_SIGNATURE      0x50450000  // PE
#define EFI_IMAGE_TE_SIGNATURE      0x565A  // ZV
#else
#define EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC 0x10b
#define EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC 0x20b

#define EFI_IMAGE_DOS_SIGNATURE	    0x5A4D  // MZ
#define EFI_IMAGE_NT_SIGNATURE      0x4550  // PE
#define EFI_IMAGE_TE_SIGNATURE      0x5A56  // VZ
#endif

#define CPU_TYPE_MIPS	((cpu_type_t) 8)
#define CPU_TYPE_SPARC	((cpu_type_t) 14)
#define CPU_TYPE_ALPHA	((cpu_type_t) 16)

void Usage(char *name)
{
	printf("EFI lipo tool\n");
	printf("This tool creates multi-architecture efi files and creates Apple FAT binaries\n");
	printf("Usage: %s <output> <efi_file1> <efi_file2> [...]\n", name);
}

uint16_t create_ms_dos_stub_checksum(uint16_t *buffer, int len)
{
	uint16_t checksum = 0;
	int donecnt = 0;

	while (donecnt < (len / 2))
	{
		checksum += buffer[donecnt];

		++donecnt;
	}

	return (0x10000 - checksum);
}

uint32_t create_pe_checksum(unsigned char *buf, int output_size)
{
	uint32_t i, v, t = 0;

	for(i = 0; i < output_size; i += 2)
	{
		if(output_size - i == 1)
			v = buf[i];
		else
			v = buf[i] + (buf[i+1] << 8);

		t += v;
		t = 0xffff & (t + (t >> 0x10));
	}

	return(0xffff & (t + (t >> 0x10)));
}

int main(int argc, char **argv)
{
  	unsigned char *outbuffer[13];
	struct fat_arch fatarch[13];
	int filesize[13];
	char isTE[13];
	ms_dos_stub_t *dosstub = NULL;
	filehdr_t *pehdr = NULL;
	uint32_t *pemagic = NULL;
	aouthdr_t *pehdr32 = NULL;
	aouthdr_64_t *pehdr64 = NULL;
	tehdr_t *tehdr = NULL;
	struct fat_header fathdr;
	FILE *fout = NULL;
	FILE *fin = NULL;
	int curfile = 0;
	int checksize = 0;
	int headerskip = 0;
	int curoffset = 0;
	uint16_t filemachine = 0;

	if (argc < 3)
	{
		Usage(argv[0]);

		return 1;
	}

#if defined(__ppc__) || defined(__ppc64__)
	fathdr.magic = FAT_MAGIC;
#else
	fathdr.magic = FAT_CIGAM;
#endif

	fathdr.nfat_arch = (uint32_t)ARCHSWAP(argc - 2);
	headerskip = sizeof(struct fat_header) + ((argc - 2) * sizeof(struct fat_arch));
	curoffset = headerskip;

	curfile = 0;
	while ((curfile < (argc - 2)) && (curfile <= 13))
	{
		fin = fopen(argv[curfile+2], "rb");

		if (fin == NULL)
		{
			printf("ERROR: Could not open input file %s (%d)\n", argv[curfile+2], (curfile+1));

			return -1;
		}

		fseek(fin, 0, SEEK_END);
		filesize[curfile] = ftell(fin);
		fseek(fin, 0, SEEK_SET);


		outbuffer[curfile] = malloc(filesize[curfile]);
		checksize = fread(outbuffer[curfile], 1, filesize[curfile], fin);
        fclose(fin);

		if (checksize != filesize[curfile])
		{
			printf("ERROR: Could not read %d bytes from %s, %d bytes read\n", checksize, argv[curfile+2], filesize[curfile]);

			return -2;
		}

		dosstub = (ms_dos_stub_t *)outbuffer[curfile];

		if (dosstub->e_magic != EFI_IMAGE_DOS_SIGNATURE)
		{
			if (dosstub->e_magic != EFI_IMAGE_TE_SIGNATURE)
			{
				printf("ERROR: File %s (nr %d) has bad DOS header magic", argv[curfile+2], (curfile+1));

				return -3;
			} else {
				isTE[curfile] = 1;
			}
		}

		if (isTE[curfile] == 0)
		{
			dosstub->e_csum = 0;
			dosstub->e_csum = create_ms_dos_stub_checksum((uint16_t *)outbuffer[curfile], dosstub->e_lfanew);

			pemagic = (uint32_t *)(outbuffer[curfile] + dosstub->e_lfanew);

			if (pemagic[0] != EFI_IMAGE_NT_SIGNATURE)
			{
				printf("ERROR: File %s (nr %d) has bad PE/TE magic (%X)", argv[curfile+2], (curfile+1), pemagic[0]);

				return -4;
			}

			pehdr = (filehdr_t *)(pemagic + 1);
			pehdr32 = (aouthdr_t *)(pehdr + 1);
			pehdr64 = (aouthdr_64_t *)(pehdr + 1);
			filemachine = pehdr->f_magic;

			if (pehdr32->magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC)
			{
				pehdr32->CheckSum = 0;
				pehdr32->CheckSum = create_pe_checksum((unsigned char *)pehdr32, (filesize[curfile] - dosstub->e_lfanew));
			}

			if (pehdr64->magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC)
			{
				pehdr64->CheckSum = 0;
				pehdr64->CheckSum = create_pe_checksum((unsigned char *)pehdr64, (filesize[curfile] - dosstub->e_lfanew));
			}
		}

		switch (PPCSWAP(filemachine))
		{
			case IMAGE_FILE_MACHINE_ARM:
				fatarch[curfile].cputype = ARCHSWAP(CPU_TYPE_ARM);
				fatarch[curfile].cpusubtype = ARCHSWAP(CPU_SUBTYPE_ARM_ALL);
				break;

			case IMAGE_FILE_MACHINE_THUMB:
				fatarch[curfile].cputype = ARCHSWAP(CPU_TYPE_ARM);
				fatarch[curfile].cpusubtype = ARCHSWAP(CPU_SUBTYPE_ARM_V7);
				break;

			case IMAGE_FILE_MACHINE_ARMV7:
				fatarch[curfile].cputype = ARCHSWAP(CPU_TYPE_ARM);
				fatarch[curfile].cpusubtype = ARCHSWAP(CPU_SUBTYPE_ARM_V7);
				break;

			case IMAGE_FILE_MACHINE_ARM64:
				fatarch[curfile].cputype = ARCHSWAP(CPU_TYPE_ARM64);
				fatarch[curfile].cpusubtype = ARCHSWAP(CPU_SUBTYPE_ARM64_ALL);
				break;

			case IMAGE_FILE_MACHINE_POWERPC:
				fatarch[curfile].cputype = ARCHSWAP(CPU_TYPE_POWERPC);
				fatarch[curfile].cpusubtype = ARCHSWAP(CPU_SUBTYPE_POWERPC_ALL);
				break;

			case IMAGE_FILE_MACHINE_POWERPCFP:
				fatarch[curfile].cputype = ARCHSWAP(CPU_TYPE_POWERPC);
				fatarch[curfile].cpusubtype = ARCHSWAP(CPU_SUBTYPE_POWERPC_ALL);
				break;

			case IMAGE_FILE_MACHINE_I386:
				fatarch[curfile].cputype = ARCHSWAP(CPU_TYPE_I386);
				fatarch[curfile].cpusubtype = ARCHSWAP(CPU_SUBTYPE_I386_ALL);
				break;

			case IMAGE_FILE_MACHINE_AMD64:
				fatarch[curfile].cputype = ARCHSWAP(CPU_TYPE_X86_64);
				fatarch[curfile].cpusubtype = ARCHSWAP(CPU_SUBTYPE_I386_ALL);
				break;

			case IMAGE_FILE_MACHINE_R3000:
			case IMAGE_FILE_MACHINE_R4000:
			case IMAGE_FILE_MACHINE_MIPS16:
			case IMAGE_FILE_MACHINE_MIPSFPU:
			case IMAGE_FILE_MACHINE_MIPSFPU16:
				fatarch[curfile].cputype = ARCHSWAP(CPU_TYPE_MIPS);
				fatarch[curfile].cpusubtype = 0;
				break;

			case IMAGE_FILE_MACHINE_R10000:
			case IMAGE_FILE_MACHINE_WCEMIPSV2:
				fatarch[curfile].cputype = ARCHSWAP(CPU_TYPE_MIPS | CPU_ARCH_ABI64);
				fatarch[curfile].cpusubtype = 0;
				break;

			case IMAGE_FILE_MACHINE_ALPHA:
				fatarch[curfile].cputype = ARCHSWAP(CPU_TYPE_ALPHA);
				fatarch[curfile].cpusubtype = 0;
				break;

			case IMAGE_FILE_MACHINE_ALPHA64:
				fatarch[curfile].cputype = ARCHSWAP(CPU_TYPE_ALPHA | CPU_ARCH_ABI64);
				fatarch[curfile].cpusubtype = 0;
				break;

			case IMAGE_FILE_MACHINE_SPARC:
				fatarch[curfile].cputype = ARCHSWAP(CPU_TYPE_SPARC);
				fatarch[curfile].cpusubtype = 0;
				break;

			case IMAGE_FILE_MACHINE_UNKNOWN:
			case IMAGE_FILE_MACHINE_IA64:
			case IMAGE_FILE_MACHINE_CEE:
			case IMAGE_FILE_MACHINE_CEF:
			case IMAGE_FILE_MACHINE_EBC:
			default:
				printf("ERROR: Unsupported PE machine type 0x%X\n", PPCSWAP(filemachine));

				return -5;
		}

        fatarch[curfile].offset = ARCHSWAP(curoffset);
        fatarch[curfile].size = ARCHSWAP(filesize[curfile]);
        fatarch[curfile].align = 0;

        curoffset += filesize[curfile];
        ++curfile;
    }

    fout = fopen(argv[1], "wb");
    
    if (fout == NULL)
    {
        printf("ERROR: Could not open output file %s\n", argv[1]);

        fclose(fout);

        return -6;
    }

    checksize = fwrite(&fathdr, sizeof(struct fat_header), 1, fout);

    curfile = 0;
    while (curfile < (argc - 2))
    {
        checksize = fwrite(&(fatarch[curfile]), sizeof(struct fat_arch), 1, fout);

        ++curfile;
    }

    curfile = 0;
    while (curfile < (argc - 2))
    {
        checksize = fwrite(outbuffer[curfile], filesize[curfile], 1, fout);
        
        ++curfile;
    }

    fclose(fout);

    return 0;
}

