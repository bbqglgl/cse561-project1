#include<iostream>
#include<cmath>
#include<unistd.h>
#include<string>
#include<stdlib.h>

using namespace std;

int main(int argc, char* argv[])
{
	char path[1001];
	string trace_path;
	double r,i,ipc;
	int rob, iq;

	if(argc < 2)
		return 0;
	trace_path = argv[1];
	FILE *fo;

	for(int w=1;w<=10;w++)
	{
		printf("%d,",w);
		for(r=0;r<=3.1;r+=0.2)
		{
			rob=round(pow(10.0, r));
			printf("%d,",rob);
		}
		printf("\n");
		for(i=0;i<=3.1;i+=0.2)
		{
			iq=round(pow(10.0, i));
			printf("%d,",iq);
			for(r=0;r<=3.1;r+=0.2)
			{
				rob=round(pow(10.0, r));
				sprintf(path,"~/aca/proj1/cse561-project1/src/ces561sim %d %d %d %s -o",rob, iq, w, trace_path.c_str());
				system(path);

				fo = fopen("./output.txt","r");

				fscanf(fo,"%lf",&ipc);
				printf("%.4lf,",ipc);

				fclose(fo);
			}
			printf("\n");
		}
		printf("\n\n");
	}
	return 0;
}
