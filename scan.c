#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "scan.h"
#define UTIL_IMPLEMENTATION
#include "utils.h"


#define MAX_CHAR 128

static char scancodes1[MAX_CHAR];
static char scancodes2[MAX_CHAR];

static char mnemonicsLetter[16];
static char mnemonics[16][66];
static char buff[64];

static volatile int altNum = 0;
static volatile int shiftFlag = 0;
static volatile int controlFlag = 0;
static volatile int altFlag = 0;
static volatile int count = 16;

static char testChar[1];
static char bafer[1];
// static int testChar;

void ispis(){
	printstr(buff);

}


void load_config(const char *scancodes_filename, const char *mnemonic_filename)
{
	char buffer2[64];

	char buffer[MAX_CHAR];
	int fd = open(scancodes_filename, O_RDONLY);

	if(fd < 0){
		write(1, "File has not been opened successfully", strlen("File has not been opened successfully"));
		_exit(1);
	}

	int len = fgets(buffer, MAX_CHAR, fd);
	int i;
	for(i = 0; i < len-1; i++){
		scancodes1[i] = buffer[i];
	}
	
	len = fgets(buffer, MAX_CHAR, fd);
	for(i = 0; i < len-1; i++){
		scancodes2[i] = buffer[i];
	}

	i = 0;
	int j;
	fd = open(mnemonic_filename, O_RDONLY);
	len = fgets(buffer, MAX_CHAR, fd);
	int end = atoi(buffer);

	while(end > i){
		len = fgets(buffer, MAX_CHAR, fd);
		mnemonicsLetter[i] = buffer[0];
		for(j = 0; j < len-1; j++){
			mnemonics[i][j] = buffer[j];
		} 
		i++;
	}


	char strFileName[MAX_CHAR];
	do {
		int i = 0;
		
		strFileName[0] = '\0';
		buffer[0] = '\0';

		write(1, "Enter the name of the file (type exit to terminate):\n", strlen("Enter the name of the file (type exit to terminate):\n"));

		int b = read(0, strFileName, MAX_CHAR);
		if(b<0){
			_exit(2);
		}

		strFileName[b-1] = '\0';

		int a = open(strFileName, O_RDONLY);
		while(strcmp(buffer, "400") && strcmp(strFileName, "exit")){
			fgets(buffer, MAX_CHAR, a);
			int bb = strlen(buffer);
			int b = atoi(buffer);
			
			// write(1, buffer, strlen(buffer)); Just to check if it has loaded
			buffer[bb-1] = '\0';
			buff[1] = '\0'; ////////
			int ret = process_scancode(b, buff);

			printstr(bafer);
		}
		write(1, "\n", 1);

	} while(strcmp(strFileName, "exit"));
}

int process_scancode(int scancode, char *buffer)
{
	int result = 1;
	asm (

		"cmp $128, %%edx;"   ////// test
		"jl ctrlCheck;"
		"jmp shift;"

		"ctrlCheck:"			////// test
		"cmp $0, %%ebx;"
		"je checkAltDown;"
		"jne ctrlAction;"

		"shift:"
		"cmp $200, %%edx;"	// <----- Shift down 
		"jne shiftUp;"
		"inc %%eax;"
		"xor %%edi, %%edi;"
		"jmp end;"

		"shiftUp:"			// <----- Shift up 
		"cmp $300, %%edx;"
		"jne ctrlDown;"
		"mov $0, %%eax;"
		"xor %%edi, %%edi;"
		"jmp end;"

		"ctrlDown:"			// <----- Ctrl down 
		"cmp $201, %%edx;"
		"jne ctrlUp;"
		"inc %%ebx;"
		"xor %%edi, %%edi;"
		"jmp end;"

		"ctrlUp:"			// <----- Ctrl up 
		"cmp $301, %%edx;"
		"jne altDown;"
		"mov $0, %%ebx;"
		"xor %%edi, %%edi;"
		"jmp end;"

		"altDown:"			// <----- Alt down 
		"cmp $202, %%edx;"
		"jne altUp;"
		"inc %%ecx;"
		"xor %%edi, %%edi;"
		"jmp end;"

		"altUp:"			// <----- Alt up 
		"cmp $302, %%edx;"
		"jne end;"
		"mov $0, %%ecx;"
		"xor %%edi, %%edi;"
		"jmp checkAltUp;"						// Ako se alt flag promenio skace na checkAltUp

		"lowerCase:"							// lower case with all flags 0
		"movw scancodes1(, %%edx, 1), %%di;"
		"jmp end;"

		"upperCase:"							// upper case with SHIFT flag set
		"movw scancodes2(, %%edx, 1), %%di;"
		"jmp end;"

		"lowerCaseAlt:"							// lower case with ALT flag set
		"movw scancodes1(, %%edx, 1), %%dx;"	// reads the value of scancodes1 at index %edx, and puts it in %dx
		"jmp endUpperCaseAlt;"

		"upperCaseAlt:"							// upper case with ALT flag set
		"movw scancodes2(, %%edx, 1), %%dx;"	// reads the value of scancodes2 at index %edx, and puts it in %dx
		"jmp endUpperCaseAlt;"					

		"endUpperCaseAlt:"						// subtracts 
		"sub $48, %%dx;" 						// subtracts 48 from %edx because %edx is an ASCII value
		"add %%edx, %%esi;"						// adds the value of %edx to %esi
		"xor %%edi, %%edi;"
		"jmp end;"


		"checkAltDown:" 						//
		"cmp $0, %%ecx;"						// Proverava da li je Alt pritisnuto
		"je checkAltUp;"						// Ako vise nije skace na checkAltUp
		"imul $10, %%esi;"						// Ako jeste %%esi mnozi sa 10
		"cmp $0, %%eax;"						// Proverava da li je lowerCaseAlt 
		"je lowerCaseAlt;"						// Proverava da li je lowerCaseAlt 
		"jne upperCaseAlt;"


		"checkAltUp:"							// Ako je alt "podignuto"		 
		"cmp $0, %%esi;"						// Proverava da li je %%esi = 0
		"je check3;"
		"mov %%si, %%di;"						// Ako nije %%si dodaje na %%di
		"xor %%esi, %%esi;"						// Setuje %%esi na 0 za sledece koriscenje
		"jmp end;" 

		"ctrlAction:"							// Ako je pritisnuto ctrl proveravamo da li je shift takodje pritisnuto
		"cmp $0, %%eax;"
		"je lowerCaseCtrl;"
		"jne upperCaseCtrl;"

		"lowerCaseCtrl:"
		"xorl %%eax, %%eax;"					// 
		"xorl %%esi, %%esi;"					// Cistimo registre koje upotrebljavamo u funkciji	
		"xorl %%ecx, %%ecx;"					//	
		"movl $16, %%ecx;"						// Setujemo %%ecx na 16 jer je to maksimalan broj mnemonica
		"leal (mnemonics), %%esi;"				// Pokazivac na mnemonike setujemo na %%esi jer koristimo lodsb
		"A: lodsb;"								// Koristimo lodsb, sto sa mesta na koje pokazuje %%edi cita vrednost i stavlja je u %%eax
		"cmpb %%al, scancodes1(, %%edx, 1);" 	// Uporedjujemo vrednost %al i scancode sa indeksom %%edx
		"je matchingMemes;"						// Ako su vrednosti jednake pozivamo funkciju matchingMemes
		"addl $65, %%esi;"						// Ako vrednosti nisu jednake onda skacemo 65 mesta kako bi presli u novi red matrice
		"loop A;"								// Loop izvrsavamo 16 puta
		"jmp end;" ///////////// ctrlAction2

		"upperCaseCtrl:"
		"xorl %%eax, %%eax;"					// 
		"xorl %%esi, %%esi;"					// Cistimo registre koje upotrebljavamo u funkciji	
		"xorl %%ecx, %%ecx;"					//	
		"movl $16, %%ecx;"						// Setujemo %%ecx na 16 jer je to maksimalan broj mnemonica
		"leal (mnemonics), %%esi;"				// Pokazivac na mnemonike setujemo na %%esi jer koristimo lodsb
		"B: lodsb;"								// Koristimo lodsb, sto sa mesta na koje pokazuje %%edi cita vrednost i stavlja je u %%eax
		"cmpb %%al, scancodes2(, %%edx, 1);" 	// Uporedjujemo vrednost %al i scancode sa indeksom %%edx
		"je matchingMemes;"						// Ako su vrednosti jednake pozivamo funkciju matchingMemes
		"addl $65, %%esi;"						// Ako vrednosti nisu jednake onda skacemo 65 mesta kako bi presli u novi red matrice
		"loop B;"								// Loop izvrsavamo 16 puta
		"jmp end;"

		"matchingMemes:"
		"lodsb;"								// Koristimo lodsb kako bi preskocili "space" koji je izmedju slova za koje se vezuje mnemonic
		"xorl %%ecx, %%ecx;"					// Cistimo %%ecx
		"leal (buff), %%edi;"					// Stavljamo pokazivac na buff u %%edi
		"mov $64, %%ecx;"						// Setujemo %%ecx(brojac) na 64

		"C:"									// Prolazimo kroz niz na koji pokazuje %%esi
		"lodsb;"								// Stavljamo vrednost na koju pokazuje %%esi u %%al
		"stosb;"								// Vrednost sa %%al stavljamo na mesto na koje pokazuje %%edi
		"loop C;"								// Loop radimo dok %%ecx ne dodje do 0, sto je 64 puta;

		"call ispis;"

		"xorl %%edi, %%edi;"					//
		"xorl %%ecx, %%ecx;"					//
		"xorl %%ebx, %%ebx;"					// Cistimo sve registre
		"xorl %%eax, %%eax;"					//
		"xorl %%esi, %%esi;"					//
		"jmp end;"

		"check3:"
		"cmp $0, %%eax;"						// Proveravamo da li je flag(%eax) za Shift setovan ili nije
		"je lowerCase;"							// Na osnovi rezultata cmp pozivamo odgovarajucu funkciju
		"jne upperCase;"

		"end:"

		//////
		: "=D" (bafer), "=a" (shiftFlag), "=b" (controlFlag), "=c"(altFlag), "=S" (altNum)	 								// 0 - result, 1 - 	shiftFlag, 	2 - controlFlag, 3 - altFlag
		: "a" (shiftFlag), "b" (controlFlag), "c"(altFlag), "d" (scancode), "S" (altNum), "g" (scancodes1),"g" (scancodes2), "g" (mnemonics),"g" (buff)// 4 - shiftFlagIN, 5 - controlFlag
		: 																													// 6 - altFlag, 	7 - scancode
																															// 8 - 	scancodes1, 9 - scancodes2, 10 - altNum
		);

	return result;
}