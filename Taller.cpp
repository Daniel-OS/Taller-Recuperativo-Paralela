#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <mpi.h>

using namespace std;

float ChaoComas(string Coma){//Elimina las comillas para devolver un numero.
    int Largo = Coma.length();
    string SinComa = Coma.substr(1,Largo-2);
    float Aint = stof(SinComa);
    return Aint;
}

bool EncontarFecha(string Fechas, string FechaB){//Busca un año en el string
    if (Fechas.find(FechaB) != string::npos)
        return true;
    return false;
}

int main(int argc, char *argv[])
{
    //Inicio lo necesario para usar MPI
    int rank;
    int p;

    MPI_Init(&argc,&argv);

    MPI_Comm_rank(MPI_COMM_WORLD,&rank);

    MPI_Comm_size(MPI_COMM_WORLD,&p);

    //Defino los datos para rectificar
    float auxX = 0;
    float auxY = 0;
    float auxD = 0;
    float sumaX = 0;
    float sumaY = 0;
    float sumaXY = 0;
    float sumaX2 = 0;
    float TDias = 6230;

    //Defino los datos para los csv
    fstream SalarioM;
    fstream Dollaru;

    SalarioM.open(argv[1],ios::in);
    Dollaru.open(argv[2],ios::in);
    string year;
    string value;
    string date;
    string amount;

    //Tomo las primeras lineas para que no molesten
    getline(SalarioM,year,';');
    getline(SalarioM,value,'\n');
    getline(Dollaru,date,';');
    getline(Dollaru,amount,'\n');

    //La maquina host toma la parte mas pesada
    //Muevo los indicadores de dolar hasta el año del sueldo minimo
    if (rank == 0){
    while(EncontarFecha(date,"1995") != true){
        getline(Dollaru,date,';');
        getline(Dollaru,amount,'\n');}
    //Extraigo y asigno los valores necesarios para rectificar, siempre y cuando los años coincidan    
    while(SalarioM.good()){
        getline(SalarioM,year,';');
        if(year.length()==0)//Al final de los archivos csv hay una linea extra en blanco
            break;          //Esta condicion evita un error. Opcionalmente, se pueden borrar de forma manual.
        getline(SalarioM,value,'\n');
        auxY=0;//Dejo los auxiliares en 0 para calcular el promedio anual del dolar en cada ciclo
        auxD=0;
        while(EncontarFecha(date,year.substr(1,4)) == true){
            auxY += ChaoComas(amount);
            auxD += 1;
            getline(Dollaru,date,';');
            getline(Dollaru,amount,'\n');
        }
        auxY = auxY/auxD;
        auxX += 1;
        auxY = ChaoComas(value) / auxY;
        sumaX += auxX;
        sumaY += auxY;
        sumaXY += auxX*auxY;
        sumaX2 += auxX*auxX;

    }
    //Envio los datos a la maquina 1
    MPI_Send(&sumaX,1,MPI_FLOAT,1,1,MPI_COMM_WORLD);
    MPI_Send(&sumaY,1,MPI_FLOAT,1,2,MPI_COMM_WORLD);
    MPI_Send(&sumaXY,1,MPI_FLOAT,1,3,MPI_COMM_WORLD);
    MPI_Send(&sumaX2,1,MPI_FLOAT,1,4,MPI_COMM_WORLD);}

    //La maquina 1 obtiene las constantes y se los pasa a la maquina 2
    if(rank==1){
    MPI_Recv(&sumaX,1,MPI_FLOAT,0,1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    MPI_Recv(&sumaY,1,MPI_FLOAT,0,2,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    MPI_Recv(&sumaXY,1,MPI_FLOAT,0,3,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    MPI_Recv(&sumaX2,1,MPI_FLOAT,0,4,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    float m = (TDias*sumaXY - sumaX*sumaY)/(TDias*sumaX2 - sumaX*sumaX);
    float b = (sumaX2*sumaY - sumaX*sumaXY)/(TDias*sumaX2 - sumaX*sumaX);
    MPI_Send(&m,1,MPI_FLOAT,2,5,MPI_COMM_WORLD);
    MPI_Send(&b,1,MPI_FLOAT,2,6,MPI_COMM_WORLD);}

    //La maquina 2 muestra la funcion Salario minimo en dollars vs Dias por pantalla
    if (rank==2){
    float m;
    float b;
    MPI_Recv(&m,1,MPI_FLOAT,1,5,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    MPI_Recv(&b,1,MPI_FLOAT,1,6,MPI_COMM_WORLD,MPI_STATUS_IGNORE);    
    printf ("=== Resultado === \n y = %f x + %f \n=== Integrante=== \n Daniel Ortega Saez \n", m, b);}

    //Cierro los documentos y finalizo MPI
    SalarioM.close();
    Dollaru.close();

    MPI_Finalize();

    return 0;
}