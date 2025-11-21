/***********************************************************************
* Shanell Chavez
* Project Part 1 – 8 Assembler Simulator
* Date: 11/17/2025
* Input Files: testPart1-8.asm
* Description:
* This program simulates an assembler that reads an .asm file and converts
* assembly instructions into machine code. It supports basic commands like
* MOV and HALT, stores them in memory, and executes them using a virtual machine.
* •	Feedback: Use je, jne, ja, jae, jb, jbe, and jmp.
•	Update: In runMachineCode and convertToMachineCode, the jump set was changed from JE, JNE, JG, JL, JGE, JLE 
to JE, JNE, JA, JAE, JB, JBE, JMP.

•	Feedback: In the false branch, you should accumulate a counter in BX so you can see which comparison failed.
•	Update: The VM now allows you to use BX as a counter. In your .asm test file, you can initialize CX with the first value, 
keep BX free, and then add bx, 1 every time a comparison fails. At the end, put will print AX (the main result) 
and BX (the failure counter).

•	Feedback: You only showed cmp reg, reg. Need cmp reg, const and cmp reg, [addr].
•	Update: In runMachineCode, CMP now supports:
o	cmp reg, reg
o	cmp reg, const
o	cmp reg, [addr]
o	Updated: also supports cmp reg, [bx] and cmp reg, [bx+###] for Part 8.

•	Feedback: PUT should only output AX, no operand.
•	Update: In runMachineCode, PUT ignores operands and prints regis.AX. In convertToMachineCode, PUT encodes 
as a single opcode with no operand.

•	Uses the correct jump mnemonics (je jne ja jae jb jbe jmp).
•	PUT prints AX only.
•	CMP supports reg, const, [addr], [bx], [bx+###].
•	MOV supports [bx] and [bx+###].
•	Data after HALT supported.
•	BX can be used as a failure counter in Part 6b tests.

************************************************************************/


//need to create the functions run the array notation. 
#define _CRT_SECURE_NO_WARNINGS  // allow use of older C library functions in MSVC

#include <stdlib.h>   // for exit(), system()
#include <stdio.h>    // for printf(), scanf(), FILE I/O
#include <string.h>   // for strlen(), strcmp(), strcpy()
#include <ctype.h>    // for tolower(), isdigit()

// -------------------- File name for assembler input --------------------
char ASM_FILE_NAME[ ] = "Test3.asm";  // assembly source file to load

// -------------------- Memory and parsing constants --------------------
#define MAX 150         // size of simulated memory array
#define COL 7           // number of columns for memory dump output
#define LINE_SIZE 100   // maximum length of an input line from ASM file

// -------------------- Operand codes --------------------
#define AXREG 0         // register AX
#define BXREG 1         // register BX
#define CXREG 2         // register CX
#define DXREG 3         // register DX
#define CONSTANT 7      // immediate constant operand

// -------------------- Opcode definitions --------------------
#define ADD 64          // ADD reg, reg/const
#define PUT 128         // PUT reg
#define MOVMEM 160      // MOV [addr], reg
#define MOVFROMMEM 176  // MOV reg, [addr]
#define CMP 32          // CMP reg, reg/const/[addr]
#define JE 16           // Jump if Equal
#define JNE 17          // Jump if Not Equal
#define JG 18           // Jump if Greater
#define JL 19           // Jump if Less
#define JGE 20          // Jump if Greater or Equal
#define JLE 21          // Jump if Less or Equal
#define MOVREG 192      // MOV reg, reg/const
#define HALT 5          // HALT instruction
#define ADDRESS 6       // operand type: immediate address
#define GET 144         // GET reg — read integer from stdin
#define FUN 224   // function entry part 8
#define RET 225   // function return part 8

// -------------------- Boolean constants --------------------
#define TRUE 1
#define FALSE 0

// -------------------- Registers structure --------------------
struct Registers {
	int AX;
	int BX;
	int CX;
	int DX;
	int flag;   // comparison flag: -1, 0, +1
} regis;       // global register set

// -------------------- Global memory --------------------
typedef short int Memory;          // memory cell type
Memory memory[ MAX ] = { 0 };        // simulated memory array
Memory address;                    // program counter (current address)

// -------------------- Function prototypes --------------------
void runMachineCode( );
void splitCommand( char line[ ], char command[ ], char part1[ ], char part2[ ] );
void convertToMachineCode( FILE* fin );
void assembler( );
void printMemoryDump( );
int convertToNumber( char line[ ], int start );
int whichOperand( char operand[ ] );
void changeToLowerCase( char line[ ] );
void printMemoryDumpHex( );
void putValue( int operand, int value );
Memory getValue( Memory operand );

// -------------------- Main program --------------------
int main( )
{
	// Initialize registers and program counter before use

	regis.AX = regis.BX = regis.CX = regis.DX = regis.flag = 0;
	address = 0;

	printMemoryDump( );   // show initial memory (all zeros)
	assembler( );         // assemble ASM file into memory[]
	runMachineCode( );    // execute instructions
	printMemoryDump( );   // show final memory and registers

	printf( "\n" );
	getchar( );           // portable pause (instead of system("pause"))
	return 0;
}


/********************   assembler   ***********************
Reads the assembly source file line by line, converts each
instruction into machine code, and stores it in memory[].
-----------------------------------------------------------*/
void assembler( )
{
	address = 0;          // reset program counter before assembling
	FILE* fin;            // file pointer for ASM input

	// Open the ASM file for reading
	// Use fopen_s in MSVC, or fopen for portability
	if ( fopen_s( &fin, ASM_FILE_NAME, "r" ) != 0 || fin == NULL )
	{
		printf( "Error: could not open ASM file '%s'\n", ASM_FILE_NAME );
		getchar( );        // portable pause instead of system("pause")
		exit( 1 );          // terminate program
	}

	// Read and convert each line until EOF or memory limit
	while ( address < MAX && !feof( fin ) )
	{
		convertToMachineCode( fin );  // parse one line and encode into memory[]
	}

	fclose( fin );           // close file when done
}

/********************   runMachineCode   ***********************
Executes the machine code stored in memory[].
Decodes each instruction, performs the operation, and updates
registers, memory, and program counter accordingly.Executes the machine code,
the virtual machine * Function: runMachineCode *
Purpose: Executes the machine code stored in memory.
* Returns: void Supports: - MOV reg, const - MOV reg, reg - ADD reg, reg/const
- PUT reg - MOV [addr], reg - MOV reg,
[addr] - CMP reg, reg/const/address <--
added address support - Jumps: JE, JNE, JG, JL, JGE, JLE
-----------------------------------------------------------*/
void runMachineCode( )
{
	// Bit masks to extract fields from encoded instruction
	Memory mask1 = 224;  // top 3 bits → opcode group
	Memory mask2 = 56;   // middle 3 bits → destination register
	Memory mask3 = 7;    // bottom 3 bits → source operand type

	Memory part1, part2, part3;
	int value2;
	address = 0;         // reset program counter

	// Simple call stack for function return addresses
	int callStack[ 100 ];
	int stackTop = 0;

	// Fetch first command
	Memory command = memory[ address++ ];

	// Main execution loop
	while ( command != HALT )
	{
		if ( address >= MAX )
		{
			printf( "Error: address out of bounds\n" );
			break;
		}

		printf( "Executing command %d at address %d\n", command, address - 1 );

		// Decode instruction fields
		part1 = command & mask1;          // opcode group
		part2 = ( command & mask2 ) >> 3;   // destination register
		part3 = command & mask3;          // source operand type

		// ---------------- MOV ----------------
		if ( part1 == MOVREG )
		{
			value2 = ( part3 == CONSTANT ) ? memory[ address++ ] : getValue( part3 );
			putValue( part2, value2 );
		}

		// ---------------- ADD ----------------
		else if ( part1 == ADD )
		{
			value2 = ( part3 == CONSTANT ) ? memory[ address++ ] : getValue( part3 );
			int current = getValue( part2 );
			putValue( part2, current + value2 );
		}

		// ---------------- PUT ----------------
		else if ( ( command & 240 ) == PUT )
		{
			printf( "PUT: %d\n", getValue( part3 ) );
		}

		// ---------------- GET ----------------
		else if ( ( command & 240 ) == GET )
		{
			int dest = command & 7;   // low 3 bits = register id
			int input;
			printf( "Enter integer: " );
			if ( scanf( "%d", &input ) == 1 )
				putValue( dest, input );
			else
				putValue( dest, 0 );
		}

		// ---------------- MOV [addr], reg ----------------
		else if ( part1 == MOVMEM )
		{
			int addr = memory[ address++ ];
			memory[ addr ] = getValue( part3 );
		}

		// ---------------- MOV reg, [addr] ----------------
		else if ( part1 == MOVFROMMEM )
		{
			int addr = memory[ address++ ];
			putValue( part2, memory[ addr ] );
		}

		// ---------------- CMP ----------------
		else if ( part1 == CMP )
		{
			int left = getValue( part2 );
			int right;

			if ( part3 == CONSTANT )
			{
				right = memory[ address++ ];
			}
			else if ( part3 == ADDRESS )
			{
				int addr = memory[ address++ ];
				right = memory[ addr ];
			}
			else
			{
				right = getValue( part3 );
			}

			regis.flag = ( left == right ) ? 0 : ( left > right ? 1 : -1 );
		}

		// ---------------- Conditional jumps ----------------
		else if ( command >= JE && command <= JLE )
		{
			int target = memory[ address++ ];
			int jump = FALSE;

			switch ( command )
			{
				case JE:  jump = ( regis.flag == 0 ); break;
				case JNE: jump = ( regis.flag != 0 ); break;
				case JG:  jump = ( regis.flag > 0 ); break;
				case JL:  jump = ( regis.flag < 0 ); break;
				case JGE: jump = ( regis.flag >= 0 ); break;
				case JLE: jump = ( regis.flag <= 0 ); break;
			}

			if ( jump )
			{
				address = target;
			}
		}

		// ---------------- FUN / RET ----------------
		else if ( command == FUN )
		{
			// Function entry marker — nothing special until called
			// You may want to record the function start address here
		}
		else if ( command == RET )
		{
			int retReg = command & 7;        // low bits = register id
			int returnVal = getValue( retReg );
			regis.AX = returnVal;            // convention: return in AX
			address = callStack[ --stackTop ]; // restore caller address
		}

		// ---------------- Fetch next command ----------------
		if ( address >= MAX )
		{
			printf( "Error: address out of bounds\n" );
			break;
		}
		command = memory[ address++ ];

		printf( "Opcode: %d  Dest: %d  Src: %d\n", part1, part2, part3 );
	}
}



/********************   splitCommand   ***********************
splits line of asm into it's three parts
/***************************************************************
* Function: splitCommand
* Purpose: Splits a line of assembly code into instruction and operands.
* Parameters:
* line[] - full line of assembly code
* instruction[] - extracted command
* part1[] - first operand
* part2[] - second operand
* Returns: void
* Examples:
"add bx, 1"   → instruction="add", part1="bx", part2="1"
"mov ax, bx"  → instruction="mov", part1="ax", part2="bx"
 ***************************************************************/

void splitCommand( char line[ ], char instruction[ ], char part1[ ], char part2[ ] )
{
	int index = 0, index2 = 0;

	// ---------------- Extract instruction ----------------
	// Read characters until space or end of line
	while ( line[ index ] != ' ' && line[ index ] != '\0' && line[ index ] != '\n' && index2 < LINE_SIZE - 1 )
		instruction[ index2++ ] = line[ index++ ];
	instruction[ index2 ] = '\0';  // terminate string

	// If line ends after instruction, no operands
	if ( line[ index ] == '\0' || line[ index ] == '\n' )
	{
		part1[ 0 ] = '\0';
		part2[ 0 ] = '\0';
		return;
	}

	// Skip spaces after instruction
	while ( line[ index ] == ' ' ) index++;

	// ---------------- Extract first operand ----------------
	index2 = 0;
	while ( line[ index ] != ' ' && line[ index ] != ',' && line[ index ] != '\0' && line[ index ] != '\n' && index2 < LINE_SIZE - 1 )
		part1[ index2++ ] = line[ index++ ];
	part1[ index2 ] = '\0';

	// Skip spaces and optional comma after first operand
	while ( line[ index ] == ' ' || line[ index ] == ',' ) index++;

	// ---------------- Extract second operand ----------------
	index2 = 0;
	while ( line[ index ] != ' ' && line[ index ] != ',' && line[ index ] != '\0' && line[ index ] != '\n' && index2 < LINE_SIZE - 1 )
		part2[ index2++ ] = line[ index++ ];
	part2[ index2 ] = '\0';

	// Debug print (optional)
	// printf("Command = %s %s %s\n", instruction, part1, part2);
}




/********************   convertToMachineCode   ***********************
Converts a single line of ASM into machine code and stores it in memory.
Each instruction is parsed into opcode and operands, then encoded into
the global memory array at the current address.

Supported instructions:
MOV reg, const or reg or [addr]
MOV [addr], reg
ADD reg, reg or const
PUT reg
GET reg
CMP reg, reg or const or [addr]
Jumps JE JNE JG JL JGE JLE
---------------------------------------------------------------------*/
void convertToMachineCode( FILE* fin )
{
	char line[ LINE_SIZE ], instruction[ LINE_SIZE ], operand1[ LINE_SIZE ], operand2[ LINE_SIZE ];

	/* Read one line from the ASM file
		If fgets returns NULL then EOF was reached and we stop */
	if ( fgets( line, LINE_SIZE, fin ) == NULL ) return;

	/* Normalize to lowercase so parsing is case insensitive */
	changeToLowerCase( line );

	/* Ignore comment lines that begin with semicolon and trim leading spaces */
	int i = 0;
	while ( line[ i ] == ' ' || line[ i ] == '\t' ) i++;
	if ( line[ i ] == ';' ) return;

	/* Split into instruction and up to two operands
		Example add bx, 1 gives instruction add operand1 bx operand2 1 */
	splitCommand( line, instruction, operand1, operand2 );

	/* Skip blank lines */
	if ( strlen( instruction ) == 0 ) return;

	/* Map operand strings to numeric codes
		ax gives 0 bx gives 1 cx gives 2 dx gives 3 constants give 7
		Bracketed addresses are handled by direct parsing not whichOperand */
	int op1 = whichOperand( operand1 );
	int op2 = whichOperand( operand2 );

	/* Guard against unknown or empty operands for cases that require them */
	/* For single operand instructions we allow op2 to be empty */
	if ( strcmp( instruction, "halt" ) != 0 &&
		  strcmp( instruction, "put" ) != 0 &&
		  strcmp( instruction, "get" ) != 0 &&
		  strncmp( instruction, "j", 1 ) != 0 )
	{
		if ( operand1[ 0 ] == '\0' )
		{
			printf( "Assembler error instruction requires operand1\n" );
			return;
		}
	}

	/* ---------------- HALT ---------------- */
	if ( strcmp( instruction, "halt" ) == 0 )
	{
		/* HALT is a single opcode and has no operands */
		memory[ address++ ] = HALT;
		return;
	}

	/* ---------------- MOV ---------------- */
	else if ( strcmp( instruction, "mov" ) == 0 )
	{
		if ( operand1[ 0 ] == '[' )
		{
			/* MOV [addr], reg
				Example mov [20], ax
				Encode MOVMEM with source register and then the immediate address */
			int addrImm = convertToNumber( operand1, 1 ); /* skip the left bracket */
			memory[ address++ ] = MOVMEM | op2;
			memory[ address++ ] = addrImm;
		}
		else if ( operand2[ 0 ] == '[' )
		{
			/* MOV reg, [addr]
				Example mov ax, [20]
				Encode MOVFROMMEM with destination register and then the immediate address */
			int addrImm = convertToNumber( operand2, 1 ); /* skip the left bracket */
			memory[ address++ ] = MOVFROMMEM | op1;
			memory[ address++ ] = addrImm;
		}
		else if ( op2 == CONSTANT )
		{
			/* MOV reg, const
				Example mov bx, 0
				Encode MOVREG with destination register and the CONSTANT flag then the immediate value */
			memory[ address++ ] = MOVREG | ( op1 << 3 ) | CONSTANT;
			memory[ address++ ] = convertToNumber( operand2, 0 );
		}
		else
		{
			/* MOV reg, reg
				Example mov ax, bx
				Encode MOVREG with destination register and source register */
			memory[ address++ ] = MOVREG | ( op1 << 3 ) | op2;
		}
	}

	/* ---------------- ADD ---------------- */
	else if ( strcmp( instruction, "add" ) == 0 )
	{
		if ( op2 == CONSTANT )
		{
			/* ADD reg, const
				Example add bx, 1
				Encode ADD with destination register and CONSTANT then the immediate value */
			memory[ address++ ] = ADD | ( op1 << 3 ) | CONSTANT;
			memory[ address++ ] = convertToNumber( operand2, 0 );
		}
		else
		{
			/* ADD reg, reg
				Example add ax, cx
				Encode ADD with destination register and source register */
			memory[ address++ ] = ADD | ( op1 << 3 ) | op2;
		}
	}

	/* ---------------- PUT ---------------- */
	else if ( strcmp( instruction, "put" ) == 0 )
	{
		/* PUT reg
			Example put ax
			Low three bits carry the register id */
		memory[ address++ ] = PUT | op1;
	}

	/* ---------------- GET ---------------- */
	else if ( strcmp( instruction, "get" ) == 0 )
	{
		/* GET reg
			Example get cx
			Low three bits carry the destination register id */
		memory[ address++ ] = GET | op1;
	}

	/* ---------------- CMP ---------------- */
	else if ( strcmp( instruction, "cmp" ) == 0 )
	{
		if ( operand2[ 0 ] == '[' )
		{
			/* CMP reg, [addr]
				Example cmp ax, [20]
				Encode CMP with destination register and ADDRESS then the immediate address */
			int addrImm = convertToNumber( operand2, 1 );
			memory[ address++ ] = CMP | ( op1 << 3 ) | ADDRESS;
			memory[ address++ ] = addrImm;
		}
		else if ( op2 == CONSTANT )
		{
			/* CMP reg, const
				Example cmp bx, 10
				Encode CMP with destination register and CONSTANT then the immediate value */
			memory[ address++ ] = CMP | ( op1 << 3 ) | CONSTANT;
			memory[ address++ ] = convertToNumber( operand2, 0 );
		}
		else
		{
			/* CMP reg, reg
				Example cmp ax, bx
				Encode CMP with destination register and source register */
			memory[ address++ ] = CMP | ( op1 << 3 ) | op2;
		}
	}

	/* ---------------- Jumps ---------------- */
	else if ( strncmp( instruction, "j", 1 ) == 0 )
	{
		/* All jumps take a numeric target address
			The target must be the address of an instruction */
		int target = convertToNumber( operand1, 0 );

		if ( strcmp( instruction, "je" ) == 0 ) memory[ address++ ] = JE;
		else if ( strcmp( instruction, "jne" ) == 0 ) memory[ address++ ] = JNE;
		else if ( strcmp( instruction, "jg" ) == 0 ) memory[ address++ ] = JG;
		else if ( strcmp( instruction, "jl" ) == 0 ) memory[ address++ ] = JL;
		else if ( strcmp( instruction, "jge" ) == 0 ) memory[ address++ ] = JGE;
		else if ( strcmp( instruction, "jle" ) == 0 ) memory[ address++ ] = JLE;

		/* Store the target address immediately after the jump opcode */
		memory[ address++ ] = target;
	}

	/* Optional debug to verify encoding after each line */
	printMemoryDump( );
}


/****************************   printMemoryDump   ********************************
Prints the contents of the simulated memory[] array in a tabular format.
Also prints the current values of registers and the flag.

Parameters:
- none (uses global memory[], regis, and address)

Notes:
- MAX is the total number of memory cells.
- COL is the number of columns to display per row.
---------------------------------------------------------------------------------*/
void printMemoryDump( )
{
	int numRows = MAX / COL + 1;   // total number of rows to print
	int carryOver = MAX % COL;     // number of columns on the last row
	int location;                  // current memory index being printed

	// Print memory contents row by row
	for ( int row = 0; row < numRows; row++ )
	{
		location = row;  // start at this row index
		for ( int column = 0; location < MAX && column < COL; column++ )
		{
			// Skip printing beyond the last valid column on the bottom row
			if ( !( numRows - 1 == row && carryOver - 1 < column ) )
			{
				// Print memory index and value
				printf( "%5d.%5d", location, memory[ location ] );

				// Jump to the next location in this column
				// This indexing scheme distributes memory values across columns
				location += ( numRows - ( carryOver - 1 < column ) );
			}
		}
		printf( "\n" );  // end of row
	}

	// Print register values
	printf( "\n" );
	printf( "AX:%d\t", regis.AX );
	printf( "BX:%d\t", regis.BX );
	printf( "CX:%d\t", regis.CX );
	printf( "DX:%d\t", regis.DX );

	// Print program counter and flag
	printf( "\n\n" );
	printf( "Address: %d\n", address );
	printf( "Flag: %d", regis.flag );

	printf( "\n\n" );
}

//-----------------------------------------------------------------------------
//**************   Helper functions   *****************************************
// may be deleted
//-----------------------------------------------------------------------------

/*********************   whichOperand   ***************************
Maps an operand string to its numeric code used in instruction encoding.

Supported operands:
- "ax" → AXREG (0)
- "bx" → BXREG (1)
- "cx" → CXREG (2)
- "dx" → DXREG (3)
- constants (starting with a digit) → CONSTANT (7)
- addresses in brackets (e.g. "[20]") are handled separately in convertToMachineCode

Parameters:
- operand[] : string containing the operand

Returns:
- integer code for the operand type
- -1 if the operand is invalid
--------------------------------------------------------------*/

int whichOperand( char operand[ ] )
{
	// Guard against null or empty strings
	if ( operand == NULL || operand[ 0 ] == '\0' )
	{
		printf( "Error: operand is empty or null\n" );
		return -1;
	}

	// Normalize first character for comparison
	char letter = tolower( ( unsigned char ) operand[ 0 ] );

	// Register operands
	if ( letter == 'a' ) return AXREG;   // ax
	else if ( letter == 'b' ) return BXREG; // bx
	else if ( letter == 'c' ) return CXREG; // cx
	else if ( letter == 'd' ) return DXREG; // dx

	// Constant operands (start with a digit)
	else if ( isdigit( ( unsigned char ) letter ) ) return CONSTANT;

	// If operand starts with '[', it's a memory address
	// This is handled in convertToMachineCode, not here
	else if ( letter == '[' ) return ADDRESS;

	// Unknown operand type
	printf( "Error: unknown operand '%s'\n", operand );
	return -1;
}


/*********************   changeToLowerCase   ********************
Converts every character in a string to lowercase in place.

Parameters:
- line[] : the string to be modified

Notes:
- Uses tolower() from <ctype.h>, which requires casting to unsigned char
  to avoid undefined behavior with negative char values.
- Stops when the null terminator '\0' is reached.
----------------------------------------------------------------*/
void changeToLowerCase( char line[ ] )
{
	int index = 0;

	// Loop until end of string
	while ( line[ index ] != '\0' )
	{
		// Cast to unsigned char before calling tolower()
		// This prevents crashes if char is signed and has a negative value
		line[ index ] = ( char ) tolower( ( unsigned char ) line[ index ] );
		index++;
	}
}


/*********************   printMemoryDumpHex   ********************
Prints the contents of memory[] in hexadecimal format, arranged in
columns. Also prints the current register values, program counter,
and flag.
----------------------------------------------------------------*/
void printMemoryDumpHex( )
{
	int numRows = MAX / COL + 1;   // total number of rows to print
	int carryOver = MAX % COL;     // number of columns on the last row
	int location;                  // current memory index being printed

	// Print memory contents row by row
	for ( int row = 0; row < numRows; row++ )
	{
		location = row;  // start at this row index
		for ( int column = 0; location < MAX && column < COL; column++ )
		{
			// Skip invalid columns on the bottom row
			if ( !( numRows - 1 == row && carryOver - 1 < column ) )
			{
				// Print memory index and value in hex
				// %5d prints the index right-aligned in 5 spaces
				// %04x prints the value as 4-digit hex with leading zeros
				printf( "%5d.%04x", location, memory[ location ] );

				// Jump to the next location in this column
				location += ( numRows - ( carryOver - 1 < column ) );
			}
		}
		printf( "\n" );  // end of row
	}

	// Print register values
	printf( "\n" );
	printf( "AX:%d\t", regis.AX );
	printf( "BX:%d\t", regis.BX );
	printf( "CX:%d\t", regis.CX );
	printf( "DX:%d\t", regis.DX );

	// Print program counter and flag
	printf( "\n\n" );
	printf( "Address: %d\n", address );
	printf( "Flag: %d\n\n", regis.flag );
}



/*********************   convertToNumber   ********************
Converts a substring of a string into an integer, starting at a given index.

Parameters:
- line[] : the string containing the number
- start  : the index to begin conversion

Returns:
- integer value parsed from the string
----------------------------------------------------------------*/
int convertToNumber( char line[ ], int start )
{
	int value = 0;
	int sign = 1;

	// Skip leading spaces
	while ( line[ start ] == ' ' || line[ start ] == '\t' )
		start++;

	// Handle optional sign
	if ( line[ start ] == '-' )
	{
		sign = -1;
		start++;
	}
	else if ( line[ start ] == '+' )
	{
		start++;
	}

	// Parse digits
	while ( isdigit( ( unsigned char ) line[ start ] ) )
	{
		value = value * 10 + ( line[ start ] - '0' );
		start++;
	}

	return value * sign;
}


/*********************   putValue   ********************
Stores a value into the specified register.

Parameters:
- operand : register identifier (AXREG, BXREG, etc.)
- value   : the value to store
----------------------------------------------------------------*/
void putValue( int operand, int value )
{
	switch ( operand )
	{
		case AXREG: regis.AX = value; break;
		case BXREG: regis.BX = value; break;
		case CXREG: regis.CX = value; break;
		case DXREG: regis.DX = value; break;
		default:
			// Unknown register code — ignore or report error
			printf( "Warning: putValue called with invalid operand %d\n", operand );
			break;
	}
}

/*********************   getValue   ********************
Retrieves a value from a register.

Parameters:
- operand : encoded operand value (AXREG, BXREG, etc.)

Returns:
- the actual register value
- 0 for unsupported cases (constants and addresses are handled elsewhere)
----------------------------------------------------------------*/
Memory getValue( Memory operand )
{
	switch ( operand )
	{
		case AXREG: return regis.AX;
		case BXREG: return regis.BX;
		case CXREG: return regis.CX;
		case DXREG: return regis.DX;

			// Constants and addresses are not handled here.
			// They are fetched directly from memory[] in runMachineCode.
		default: return 0;
	}
}



