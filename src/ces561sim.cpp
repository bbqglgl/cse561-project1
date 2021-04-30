#include<iostream>
#include<vector>
#include<queue>
#include<list>
#include<cstdlib>
#include<cstring>

using namespace std;
typedef unsigned int UINT;
const UINT ARCH_REG_SIZE = 67;
const UINT PHY_REG_SIZE = ARCH_REG_SIZE * 2;
const UINT LATENCY_TYPE[3] = { 1,2,5 };

UINT WIDTH;
UINT IQ_SIZE;
UINT ROB_SIZE;


UINT mapTable[ARCH_REG_SIZE + 1];
bool readyTable[PHY_REG_SIZE + 1];

typedef struct _reorder
{
	UINT inst_num;
	UINT old_reg;
	bool done;
	_reorder(UINT _inst) : inst_num(_inst), old_reg(PHY_REG_SIZE), done(false) {};
}REORDER;

typedef struct _inst
{
	UINT type;
	UINT arch_dest;
	UINT arch_src1, arch_src2;

	UINT phy_dest;
	UINT phy_src1, phy_src2;

	REORDER* reorder_ptr;
	UINT log[10];

	_inst(UINT _type, UINT _dest, UINT _src1, UINT _src2) : type(_type), arch_dest(_dest), arch_src1(_src1), arch_src2(_src2) {};
}INST;


typedef struct _issue
{
	UINT inst_num;
	UINT phy_dest;
	UINT phy_src1, phy_src2;
	bool* ready_src1, * ready_src2;
	UINT bdy;
	_issue(UINT _inst, UINT _dest, UINT _src1, UINT _src2, UINT _bdy) : inst_num(_inst), phy_dest(_dest), phy_src1(_src1), phy_src2(_src2), bdy(_bdy)
	{
		ready_src1 = readyTable + _src1;
		ready_src2 = readyTable + _src2;
	};

}ISSUE;


vector<INST> instructions;

queue<UINT> freePhyReg;
list<ISSUE> issueQueue;
queue<REORDER> reorderBuffer;

UINT cycle = 0;

string inputName;
FILE* in;

queue<UINT> DE, RN, DI, RR, WB;
list<pair<UINT, UINT> > execute_list;

bool simpleOutput = false;


void initReg()
{
	int i = 0;

	for (i = 0; i < ARCH_REG_SIZE; i++)
	{
		mapTable[i] = i;
		readyTable[i] = true;
	}

	for (i = ARCH_REG_SIZE; i < PHY_REG_SIZE; i++)
		freePhyReg.push(i);

	//init for reg num -1
	mapTable[ARCH_REG_SIZE] = PHY_REG_SIZE;
	readyTable[PHY_REG_SIZE] = true;
}
void commit();
void writeback();
void execute();
void regRead();
void issue();
void dispatch();
void rename();
void decode();
void fetch();
bool advance_cycle();

int main(int argc, char* argv[])
{
	// ./cse561sim <ROB_SIZE> <IQ_SIZE> <WIDTH> <tracefile>
	if (argc < 5)
	{
		printf("Arguments format is not avliable!!\n ex)%s <ROB_SIZE> <IQ_SIZE> <WIDTH> <tracefile>\n", argv[0]);
		return -1;
	}

	ROB_SIZE = atoi(argv[1]);
	IQ_SIZE = atoi(argv[2]);
	WIDTH = atoi(argv[3]);
	inputName = argv[4];
	if(argc == 6 && strcmp("-o", argv[5])==0)
		simpleOutput=true;

	in = fopen(inputName.c_str(), "r");

	if (in == NULL)
	{
		printf("Can not open trace file!!\nCheck file name!\n");
		return -2;
	}

	instructions.reserve(100010);
	initReg();

	do
	{
		commit();
		writeback();
		execute();
		regRead();
		issue();
		dispatch();
		rename();
		decode();
		fetch();
	} while (advance_cycle());

	if(simpleOutput)
	{
		FILE *out = fopen("./output.txt","w");
		fprintf(out,"%.4lf",(double)instructions.size() / (double)cycle);
		fclose(out);
		return 0;
	}
	printf("=============================\n");
	printf("Instruction : %d\n", (int)instructions.size());
	printf("Cycles : %d\n", cycle);
	printf("IPC : %.2f\n", (double)instructions.size() / (double)cycle);
	return 0;
}

void print_info(int num)
{
	INST t = instructions[num];
	if (t.arch_dest == ARCH_REG_SIZE)
		t.arch_dest = -1;
	if (t.arch_src1 == ARCH_REG_SIZE)
		t.arch_src1 = -1;
	if (t.arch_src2 == ARCH_REG_SIZE)
		t.arch_src2 = -1;

	if(simpleOutput)
		return;

	printf("%d fu{%d} src{%d,%d} dst{%d} FE{%d,%d} DE{%d,%d} RN{%d,%d} DI{%d,%d} IS{%d,%d} RR{%d,%d} EX{%d,%d} WB{%d,%d} CM{%d,%d}\n",num, t.type, t.arch_src1, t.arch_src2, t.arch_dest,
		t.log[0], t.log[1] - t.log[0],
		t.log[1], t.log[2] - t.log[1],
		t.log[2], t.log[3] - t.log[2],
		t.log[3], t.log[4] - t.log[3],
		t.log[4], t.log[5] - t.log[4],
		t.log[5], t.log[6] - t.log[5],
		t.log[6], t.log[7] - t.log[6],
		t.log[7], t.log[8] - t.log[7],
		t.log[8], t.log[9] - t.log[8]);
}

void commit()
{
	const UINT log_index = 8;
	int num;
 	for (int i =0; i<WIDTH && !reorderBuffer.empty() && reorderBuffer.front().done ; i++)
	{
		if(reorderBuffer.front().old_reg != PHY_REG_SIZE)
			freePhyReg.push(reorderBuffer.front().old_reg);
		num = reorderBuffer.front().inst_num;
		instructions[num].log[log_index + 1] = cycle + 1;
		print_info(num);
		reorderBuffer.pop();
	}
}

void writeback()
{
	const UINT log_index = 7;
	int num;
	while (!WB.empty())
	{
		num = WB.front();
		WB.pop();

		instructions[num].reorder_ptr->done = true;

		instructions[num].log[log_index + 1] = cycle + 1;
	}
}
void execute()
{
	const UINT log_index = 6;
	auto iter = execute_list.begin();

	while (iter != execute_list.end() && WB.size() < WIDTH * 5)
	{
		(iter->second)--;
		if (iter->second == 0)
		{
			WB.push(iter->first);
			instructions[iter->first].log[log_index + 1] = cycle + 1;
			readyTable[instructions[iter->first].phy_dest] = true;
			execute_list.erase(iter++);
		}
		else
			iter++;
	}
}

void regRead()
{
	const UINT log_index = 5;
	int num;
	while (execute_list.size() < WIDTH * 5 && !RR.empty())
	{
		num = RR.front();
		RR.pop();

		instructions[num].log[log_index + 1] = cycle + 1;

		execute_list.push_back(make_pair(num, LATENCY_TYPE[instructions[num].type]));
	}
}

void issue()
{
	const UINT log_index = 4;
	auto iter = issueQueue.begin();

	while (iter != issueQueue.end() && RR.size() < WIDTH)
	{
		if (*(iter->ready_src1) && *(iter->ready_src2))
		{
			RR.push(iter->inst_num);
			instructions[iter->inst_num].log[log_index + 1] = cycle + 1;
			issueQueue.erase(iter++);
		}
		else
			iter++;
	}
}

void dispatch()
{
	const UINT log_index = 3;
	static int _bdy = 0;
	if (DI.empty())
		return;

	int num;
	INST* inst_ptr;
	while(!DI.empty() && IQ_SIZE - issueQueue.size() > 0)
	{
		num = DI.front();
		DI.pop();
		inst_ptr = &instructions[num];
		ISSUE issue(num , inst_ptr->phy_dest, inst_ptr->phy_src1, inst_ptr->phy_src2, _bdy++);
		issueQueue.push_back(issue);

		instructions[num].log[log_index + 1] = cycle + 1;
	}
}
void rename()
{
	const UINT log_index = 2;
	int num;
	while (!freePhyReg.empty() && DI.size() < WIDTH && !RN.empty())
	{
		num = RN.front();
		RN.pop();

		instructions[num].phy_src1 = mapTable[instructions[num].arch_src1];
		instructions[num].phy_src2 = mapTable[instructions[num].arch_src2];
		instructions[num].phy_dest = mapTable[instructions[num].arch_dest];

		if (instructions[num].arch_dest != ARCH_REG_SIZE)
		{
			instructions[num].reorder_ptr->old_reg = instructions[num].phy_dest;
			instructions[num].phy_dest = freePhyReg.front();
			mapTable[instructions[num].arch_dest] = freePhyReg.front();
			readyTable[freePhyReg.front()] = false;
			freePhyReg.pop();
		}

		instructions[num].log[log_index + 1] = cycle + 1;

		DI.push(num);
	}
}

void decode()
{
	const UINT log_index = 1;
	int num;
	while (!DE.empty() && RN.size() < WIDTH)
	{
		num = DE.front();
		DE.pop();
		
		instructions[num].log[log_index + 1] = cycle + 1;

		RN.push(num);
	}
}

void fetch()
{
	const UINT log_index = 0;
	static UINT INST_NUM = 0;
	static bool isEOF = false;
	char pc[32];
	int td, ts1, ts2, type;

	if (DE.size() >= WIDTH)
		return;
	if (reorderBuffer.size() >= ROB_SIZE)
		return;

	for (int i = 0; i < WIDTH && !isEOF && reorderBuffer.size() < ROB_SIZE && DE.size() < WIDTH; i++)
	{
		if (fscanf(in, "%s %d %d %d %d", pc, &type, &td, &ts1, &ts2) != EOF)
		{
			if (td == -1)
				td = ARCH_REG_SIZE;
			if (ts1 == -1)
				ts1 = ARCH_REG_SIZE;
			if (ts2 == -1)
				ts2 = ARCH_REG_SIZE;

			int num = INST_NUM++;

			REORDER tr(num);
			reorderBuffer.push(tr);

			INST ti(type, td, ts1, ts2);
			ti.reorder_ptr = &(reorderBuffer.back());
			ti.log[log_index] = cycle;
			ti.log[log_index + 1] = cycle + 1;

			instructions.push_back(ti);

			DE.push(num);
		}
		else
			isEOF = true;
	}
}

bool advance_cycle()
{
	if (reorderBuffer.empty())
		return false;
	cycle++;
	return true;
}
