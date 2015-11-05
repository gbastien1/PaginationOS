#include <cstdlib>
#include <iostream>
#include <fstream>
using namespace std;


#define CHAR unsigned char
#define FAULT 255
const CHAR TAILLE		= 32;
const CHAR TAILLEPAGE	= 4; 


class CTableEntry
{
private:
	CHAR Segment;
	int NoSwap;
	CHAR Cadre;
	CHAR RWX;
	char Process;
	CHAR R_Table; //Justification
	CHAR M_Table; //Justification

public:
	CTableEntry(CHAR segment, int noSwap, CHAR cadre, CHAR rwx, char process)
	{
		Segment =	segment;
		NoSwap =	noSwap;
		Cadre =		cadre;
		RWX =		rwx;
		Process =	process;
		M_Table = 0; //Justification
		R_Table = 0; //Justification
	}

	CHAR getCadre()		{	return Cadre;		}
	CHAR getSegment()	{	return Segment;		}
	int getNoSwap()	{	return NoSwap;		}
	char getProcess()	{	return Process;		}
	CHAR getRTable()	{	return R_Table;		} //Justification
	CHAR getMTable()	{   return M_Table;		} //Justification

	//Justification
	void AddRTable(bool r) 
	{
		R_Table >>= 1;
		if (r) R_Table += 128; 
	}

	//Justification
	void AddMTable(bool m)
	{
		M_Table >>= 1;
		if (m) M_Table += 128;
	}

	void setCadre(CHAR cadre)	{	Cadre = cadre;	}
};


class CProcesseur
{
	CHAR*			RAM;
	CTableEntry**	TableDesCadres;

	CHAR			Registre;
	CHAR			PC;				// Program Counter
	CHAR			State;
	CTableEntry***	PageTable;

public :
	CProcesseur()
	{
		RAM = new CHAR[TAILLE*2];
		for (int x=0; x < TAILLE*2; x++) RAM[x] = FAULT;

		TableDesCadres = new CTableEntry*[TAILLE*
			2/TAILLEPAGE];
		for (int x=0; x < TAILLE*2/TAILLEPAGE; x++) 
			TableDesCadres[x] = NULL;

		Registre	= 0;
		PC			= 0;
		State		= 'E';
	}

	~CProcesseur()						{ delete RAM;			}	

	CHAR	retPC()						{ return PC;			}
	CHAR	retRegistre()				{ return Registre;		}
	CHAR	retState()					{ return State;			}
	void setPC(CHAR pc)					{ PC = pc;				}
	void setRegistre(CHAR registre)		{ Registre = registre;	}
	void setState(CHAR state)			{ State = state;		}
	void setTable(CTableEntry*** table)	{ PageTable = table;	}
	void setRAM(CHAR indice, CHAR val)	{ RAM[indice] = val;	}

	CHAR resolve(CHAR data, CHAR segment)	
	{
		CHAR page = (CHAR)((data & 252)/4);		// ou data >> 2
		CHAR deplacement = (data & 3);

		CHAR cadre = PageTable[segment][page]->getCadre();

		return (cadre != FAULT ? 
				PageTable[segment][page]->getCadre()*TAILLEPAGE + deplacement : 
				FAULT);
	}
	

//*************************
// MOD START 
CTableEntry* getPageFault()	
	{
		if(PageTable[0][(PC & 252)/4]->getCadre() == FAULT)
			return PageTable[0][(PC & 252)/4];
		
		CHAR instruction = resolve(PC, 0);
		CHAR no	= (RAM[instruction] & 224) >> 5;
		CHAR data (RAM[instruction] & 31);
		
		if ((no >=1 && no <= 4)) 
		{
			CHAR page = (CHAR)((data & 252)/4);
			if (PageTable[1][page]->getCadre() == FAULT)
				return PageTable[1][page];
		}
		if (no == 7)
		{
			if (data == 0)
			{
				CHAR page = (CHAR)((Registre & 31 & 252)/4);
				if (PageTable[1][page]->getCadre() == FAULT)
					return PageTable[1][page];
			}
		}

		return NULL;					// pas de défaut de page à cette instruction
	}
	
	void MiseAJour(char currentInstruction, char currentCadre)
	{
		for (int i=32; i < TAILLE/TAILLEPAGE + 32; i++){
			bool r = 0;
			bool m = 0;

			if (i == currentCadre) {
				switch (currentInstruction) {
					case 1: r = 1; break;
					case 2: m = 1; break;
					case 3:
						r = 1; 
						m = 1;
						break;
					case 4:
						r = 1; 
						m = 1;
						break;
					case 5: r = 1; break;
					case 7: r = 1; break;
				}
			}

			TableDesCadres[i]->AddMTable(m);
			TableDesCadres[i]->AddRTable(r);
		}
	}

	void loadPage(CTableEntry* page)
	{
		CHAR cadreLibre = getCadreLibre(page->getSegment());
	
		page->setCadre(cadreLibre);
		TableDesCadres[cadreLibre] = page;

		ifstream file("Swap.cpp", ios::binary);		
		file.seekg(page->getNoSwap());

		for (int y=0; y < TAILLEPAGE; y++)
			file >> std::noskipws >> RAM[cadreLibre*TAILLEPAGE+y];
		file.close();
	}

	CHAR getCadreLibre(CHAR segment)
	{
		CHAR smallestValue;
		CHAR cadre;

		//Find the lowest page
		for (int i = 0; i < TAILLE/TAILLEPAGE; i++) {
			if (TableDesCadres[i]->getRTable() < smallestValue) {
				smallestValue = TableDesCadres[i]->getRTable();
				cadre = i;
			}
		}

		cadre = cadre + segment*TAILLE/TAILLEPAGE;
		
		if (TableDesCadres[cadre] != NULL)
		{
			save(cadre);
			TableDesCadres[cadre]->setCadre(FAULT);
			TableDesCadres[cadre] = NULL;
		}				
		return cadre;
	}

	void save(CHAR cadre) 
	{
		ofstream file("Swap.cpp", ios::binary | ios::out | ios::in | ios::ate);
		if (file.is_open() == 0)
		{
			cout << "Erreur ecriture" << endl;
			exit(-1);
		}
		
		file.seekp(TableDesCadres[cadre]->getNoSwap());
		
		for (int y = 0; y < TAILLEPAGE; y++)
			file << RAM[cadre*TAILLEPAGE+y] << flush;
		
		file.close();
	}
// MOD END 
//*************************


	void killProcess()
	{	
		for (int y = 0; y < 2; y++)
		{
			for (int x = 0; x < TAILLE/TAILLEPAGE; x++)		
				if(PageTable[y][x]->getCadre() != FAULT)
					TableDesCadres[PageTable[y][x]->getCadre()] = NULL;
		}
	}

	void run()
	{
		CHAR newPC = resolve(PC,0);

		char no		= (RAM[newPC] & 224) >> 5;
		CHAR data	= (RAM[newPC] & 31);
	
		CHAR adresseProg = resolve(data,0);
		CHAR adresseData = resolve(data,1);

		MiseAJour(no, adresseData); //Justification

		if (State != 'E') return;		// Pas exécutable
		switch (no)
		{
		case 0	: // CPY  valeur	: Copie valeur dans le registre.
				Registre = data;
				PC++;
				break;
		case 1	: // RED [adresse]	: Copie adresse dans le registre.
				Registre = RAM[adresseData];
				PC++;
				break;
		case 2	: // WRT [adresse]	: Copie le registre dans adresse.
				RAM[adresseData] = Registre;
				PC++;
				break;
		case 3	: // ADD [adresse]	: Additionne la valeur dans adresse au registre.
				Registre += RAM[adresseData];
				PC++;
				break;
		case 4	: // SUB  adresse	: Soustrait la valeur dans adresse au registre.
				Registre -= RAM[adresseData];
				PC++;
				break;
		case 5	: // JMP  adresse	: Effectue un saut à l'adresse dans le registre et copie adresse dans le registre.
				PC = Registre;
				Registre = data;
				break;
		case 6	: // JEQ  adresse	: Effectue un saut (jump) à adresse si la valeur du registre est égale à 0.
				if (Registre == 0)	PC = data;
				else				PC++;
				break;
		case 7	: //INT valeur	Effectue l'interruption valeur.
				if (data == 0)
				{
					adresseData = resolve(Registre & 31, 1);
					cout << (RAM[adresseData] != 0 ? (char)(';' + RAM[adresseData]) : ' ');
				}
				if(data==1) State = 'D';
				PC++;
				break;
		}
	}

	void afficherRAM(bool entete = true)
	{
		if (entete)
		{
			cout << endl;
			for (int x = 0; x < TAILLE*2 / 4; x++)
			{
				if(TableDesCadres[x] != NULL)
				{
					cout << (int)(x%10) << flush;
					cout << ":" << flush;
					cout << TableDesCadres[x]->getProcess() << flush;
					cout << (int)TableDesCadres[x]->getSegment() << flush;					
					cout << " ";
				}
				else
					cout << "-" << (x % 10) << "-- ";
			}
		}
		for (int x = 0; x < TAILLE; x++)
		{
			if (x % 4 == 0 && x != 0) cout << " ";
			cout << RAM[x] % 10 ;
		}

		for (int x = TAILLE; x < 2*TAILLE ; x++)
		{
			if (x % 4 == 0 && x != TAILLE) cout << " ";
			cout << (RAM[x] != 0 ? (char)(';' + RAM[x]) : '_');	
		}
		cout << endl;
	}
};


class CProcess
{
	static int NoProcess;
	char	ID;
	CHAR	PC;
	char	State;
	CHAR	Registre;

	CTableEntry*** Table;

public:
	CProcess(char id, char state, const char *fileName)
	{
		ID			= id;
		PC			= 0;
		State		= state;
		Registre	= 0;
	
		Table = new CTableEntry**[2];
		for (int y = 0; y < 2; y++)
		{
			Table[y] = new CTableEntry*[TAILLE/TAILLEPAGE];
			for (int x = 0; x < TAILLE/TAILLEPAGE; x++)		
				Table[y][x]= new CTableEntry(y,						// segment 
											NoProcess*64+y*32+x*TAILLEPAGE,// NoSwap
											FAULT,					// pas de cadre
											1 |						// read
											(y==1 ? 2 : 0) |			// write
											(y!=1 ? 4 : 0),			// execute
											id );					// id				
		}
		prepareSwap(fileName);
		NoProcess ++;
	}

	CProcess()
	{
		for (int y = 0; y < 2; y++)
		{
			for (int x = 0; x < TAILLE/TAILLEPAGE; x++)
				delete Table[y][x];
			delete Table[y];
		}
		delete Table;
	}

	void prepareSwap(const char* fileName)
	{
		ifstream fileIn(fileName);
		if (fileIn.is_open() == false) 
		{
			cout  << "fichier introuvable" << endl;
			exit(-1);
		}

		ofstream fileOut("Swap.cpp", ios::app | ios::binary);		
		fileOut.seekp(NoProcess * 64);
		int compte = 0; 
		while(!fileIn.eof())
		{
			compte ++;
			int tamp1;
			fileIn >> tamp1;
			CHAR tamp2 = (CHAR)tamp1;
			fileOut << tamp2;
		}
		fileOut.close();
		fileIn.close();	  
	}

	char retID()				{	return ID;			}
	char retPC()				{	return PC;			}
	char retState()				{	return State;		}
	char retRegisre()			{	return Registre;	}
	CTableEntry*** retTable()	{	return Table;	}

	void setID(char id)				{	ID = id;			}
	void setPC(CHAR pc)				{	PC = pc;			}
	void setState(char state)		{	State = state;		}
	void setRegisre(CHAR registre)	{	Registre = registre;}
};


int CProcess::NoProcess = 0;


CProcess* getNewProcess();
void setActiveProcess(CProcess *process);
void executerUneInstruction(CProcess* process);
CHAR getNextProcess(CHAR nbrProcess);


const int NombreProcess = 12;
CProcesseur CPU;
CProcess** P = new CProcess*[NombreProcess];


int main()
{
	ofstream fileOut("Swap.cpp", ios::trunc);
	fileOut.close();

	//CProcess** P = new CProcess*[NombreProcess];  //JUSTIFICATION
	for (int x = 0; x < NombreProcess; x++) P[x] = NULL;
	
	int IterMax = -1;
	while (IterMax++ < 50000)
	{ 
		//CPU.afficherRAM();
		int No = getNextProcess(NombreProcess);
		if (P[No] == NULL)	P[No] = getNewProcess();
		else
		{
			switch (P[No]->retState())
			{
			case 'E' :		// executer
				executerUneInstruction(P[No]);
				break;
			case 'D' :		// kill process
				cout << "<" << P[No]->retID() << ">" << endl;
				setActiveProcess(P[No]);
				CPU.killProcess();
				delete P[No];
				P[No] = NULL;
				break;
			}
		}
	}
}

CProcess* getNewProcess()
{
	static int NoName=0;
	int type = rand()%2;
	CHAR name = ('A' + NoName <= 'Z' ? (CHAR)('A'+NoName++) : 'A' + (NoName = 0)); // Hi Hi Hi
	switch (type)
	{
		case 0 : return new CProcess(name, 'E', "file1.txt");	break;
		case 1 : return new CProcess(name, 'E', "file2.txt");	break;
		case 2 : return new CProcess(name, 'E', "file3.txt");	break;
		case 3 : return new CProcess(name, 'E', "file4.txt");	break;
		case 4: return new CProcess(name, 'E', "file5.txt");	break;
	}
	return NULL; // arrive jamais en principe
}


void setActiveProcess(CProcess* process)
{
	CPU.setTable(process->retTable());
	CPU.setState(process->retState());			
	CPU.setRegistre(process->retRegisre());
	CPU.setPC(process->retPC());
}


void executerUneInstruction(CProcess* process)
{
	setActiveProcess(process);
		
	CTableEntry* page = NULL;
	while ((page = CPU.getPageFault()) != NULL)	CPU.loadPage(page);

	CPU.run();

	process->setPC(CPU.retPC());
	process->setRegisre(CPU.retRegistre());
	process->setState(CPU.retState());
}


CHAR getNextProcess(CHAR nbrProcess)
{
	static int numero = 0;
	return numero++ % nbrProcess;
}

