#include <opencv2\opencv.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\imgproc\imgproc.hpp>
#include <iostream>
#include <stdlib.h>
#include <conio.h>
#include <cmath>

using namespace std;
using namespace cv;

//Vetor de auxiliares
int aux[] = { 0,0,0,0,0,0,0,0,0,0 };
Mat img; //A imagem lida da WebCam é armazenada nessa matriz
Mat	model; //Uma imagem do cenário é salva nessa matriz para fins de subtração (Background Subtraction)
float norma, relacao = 1;

void configuracao() {
	string op;

	cout << "Operacao: " << endl;
	cin >> op;
	if (op == "c") { 
		float objcm, pxl; // <objcm> tamanho do objeto para calibra��o;
		cout << "Insira o Tamanho do Objeto em cm" << endl;
		cin >> objcm;
		cout << "Insira o Tamanho do Objeto em pxl ou digite zero para automatizar" << endl;
		cin >> pxl;
		if (pxl == 0) { pxl = norma; }
		relacao = objcm * (1.0) / pxl;
		cout << "Relacao prixel centimetro : " << relacao << endl;
	}
	else { system("cls");}
}

//Struct para definição da área de interesse
struct ROI {
		int col;
		int row;
};

//Struct para definição dos limiares
struct LIM {
	int sup;
	int inf;
};

//Struct pra definição das coordenadas dos pontos encontrados
struct COOR {
	int i;
	int j;
};

//Criação de uma variável do tipo ROI (Regiao de interesse) de tamanho 2 (início e fim)
struct ROI roi[2];

//Função para selecionar o ROI (Regiao de iteresse) a partir do click na tela
//Auxiliar 3 para comutar entre coordenadas inicial e final
void mouse_callback(int event, int x, int y, int flag, void *param) {
	if ((event == EVENT_LBUTTONDBLCLK) && (aux[3] == 0)) {
		cout << "(" << x << ", " << y << ")" << endl;
		roi[0].col = x;
		roi[0].row = y;
		aux[3] = 1;
	}
	else if ((event == EVENT_LBUTTONDBLCLK) && (aux[3] == 1)) {
		cout << "(" << x << ", " << y << ")" << endl;
		roi[1].col = x;
		roi[1].row = y;
		aux[3] = 0;
	}
}

int main(void) {

	struct LIM lim[2]; //Variavel lim do tipo LIM para limiarização Canny e Gaussiano
	struct COOR coor[2]; //Vetor coor de tamanho 2 para encontrar pontos na imagem (em teste)
	Mat	dif; //Nessa matriz sera armazenada a diferença entre a Matriz model e a Matriz img
	Mat gray, gray1, gray2; //Matrizes para armazenar as transformações para escalas de cinza
	Mat borda; //Matriz para armazenar a imagem de borda

	namedWindow("imColor"); // Cria janela para impressão da imagem colorida
	namedWindow("Adjusts"); // Cria janela para ajustes
	namedWindow("imBorda"); // Cria janela para impressão da imagem de bordas
	
	// Criação das barras de ajuste
	createTrackbar("UpCanny", "Adjusts", 0, 1000);
	createTrackbar("LowCanny", "Adjusts", 0, 1000);
	createTrackbar("UpGauss", "Adjusts", 0, 3);
	createTrackbar("LowGauss", "Adjusts", 0, 3);
	createTrackbar("Gray", "Adjusts", 0, 50);
	createTrackbar("Model", "Adjusts", 0, 1);

	VideoCapture cam(0); // Ativa webcam
	cam.read(model); // Inicializa a matriz modelo
	cam.read(dif); // Iniciliza a matriz dif
	cvtColor(model, gray1, COLOR_BGR2GRAY); // Conversão matriz model para escala de cinzas
	cvtColor(dif, gray2, COLOR_BGR2GRAY); // Conversão matriz dif para escala de cinzas

	while (true) { // Loop infinito

		int nivelCinza = getTrackbarPos("Gray", "Adjusts"); // A variável nivelCinza recebe o valor lido na barra de ajustes

		cam.read(img); // Leitura da imagem em modo Vivo
		cvtColor(img, gray, COLOR_BGR2GRAY); // Conversão matriz img para escala de cinzas

		setMouseCallback("imColor", mouse_callback); // Definição do ROI (Regiao de interesse)
		rectangle(img, { roi[0].col, roi[0].row, }, { roi[1].col ,roi[1].row }, { 0,255,0 }, 2, cv::LINE_AA, 0); // Insere na tela o retângulo referente ao ROI
		
		// Caso Modelo esteja na posição 1 em ajustes, é feito um novo modelo
		if (getTrackbarPos("Model", "Adjusts") == 1) {
			setTrackbarPos("Model", "Adjusts", 0);
			gray.copyTo(gray1);
		}
	
		// A matriz dif é manipulada de forma a eliminar todo o fundo
		for (int i = 0; i < img.rows; i++) {
			for (int j = 0; j < img.cols; j++) {
				if ((gray1.at<uchar>(i, j) - gray.at<uchar>(i, j)) < nivelCinza) { // Caso a subtração do elemento [i, j] da matriz modelo pelo elemento da matriz img for menor que o nivelCinza escolhido, o elemento [i, j] em dif é setado a zero 
					gray2.at<uchar>(i, j) = 0;
				} else {
					gray2.at<uchar>(i, j) = gray.at<uchar>(i,j);
				}
			}
		}
	
	// Recebe os limites da suavização gaussiana
	lim[1].inf = getTrackbarPos("LowGauss", "Adjusts");
	lim[1].sup = getTrackbarPos("UpGauss", "Adjusts");
	
	//Aplica a suavização gaussiana na matriz dif
	GaussianBlur(gray2, gray2, Size(7, 7), lim[1].inf, lim[1].sup);
	
	// Recebe os limites da detecção via metódo Canny
	lim[0].inf = getTrackbarPos("LowCanny", "Adjusts");
	lim[0].sup = getTrackbarPos("UpCanny", "Adjusts");
	
	// Conversão da matriz dif para bordas pelo método Canny
	Canny(gray2, borda, lim[0].inf, lim[0].sup, 3); 

	//flip(img, img, +1); // Inverte a imagem (epelhar)
	//flip(borda, borda, +1); // Inverte a imagem (epelhar)

	waitKey(10); // Pequeno delay

	// For que lê de cima para baixo, todas as colunas da esquerda para a direita
	// Utilizado Auxiliar 1 para encerrar busca
	for (int i = roi[0].row; i <= roi[1].row; i++) {
		for (int j = roi[0].col; j <= roi[1].col; j++) {
			if ((borda.at<uchar>(i,j) == 255)&&(aux[1]==0)) {
				coor[0].i = i;
				coor[0].j = j;
				aux[1] = 1;
			}
		}
	}

	// For que lê da esquerda para a direita todas as linhas de baixo para cima
	// Utilizado Auxiliar 2 para encerrar busca
	for (int j = roi[0].col ; j <= roi[1].col; j++) {
		for (int i = roi[1].row; i >= roi[0].row; i--) {
			if ((borda.at<uchar>(i, j) == 255) && (aux[2] == 0)) {
				coor[1].i = i;
				coor[1].j = j;
				aux[2] = 1;
			}
		}
	}
	
	// Impressão de uma linha contornando objeto
	line(img, { coor[1].j ,coor[1].i }, { coor[0].j ,coor[0].i }, { 100,100,100 }, 2, cv::LINE_AA, 0);
	
	// Impressão de quadrados indicando ponto detectado
	rectangle(img, { coor[0].j - 3 ,coor[0].i - 3 }, { coor[0].j + 3 ,coor[0].i + 3 }, { 255,0,0 }, 2, cv::LINE_AA, 0); // Imprime na tela um retangulo nas coordenadas do ponto encontrado
	rectangle(img, { coor[1].j - 3 ,coor[1].i - 3 }, { coor[1].j + 3 ,coor[1].i + 3 }, { 0,255,0 }, 2, cv::LINE_AA, 0); // Imprime na tela um retangulo nas coordenadas do ponto encontrado

	//Zera os auxiliares
	aux[1] = 0;
	aux[2] = 0;

	//Para calcular a norma
	double a = coor[0].i - coor[1].i;
	double b = coor[0].j - coor[1].j;

	// Cálculo da norma do vetor encontrado
	norma = sqrt(pow(a,2)+pow(b,2));

	//Gera String a partir da norma para ser impresso na tela
	ostringstream tamanho;
	tamanho << norma*relacao;

	//Imprime o tamanho na tela
	putText(img, tamanho.str() , { coor[1].j,coor[1].i}, CV_FONT_HERSHEY_SIMPLEX, 2, { 255,0,0 }, 2, cv::LINE_AA, 0);
	
	//Chama configuração quando uma tecla é pressionada
	if (_kbhit()) {
		configuracao();
	}

	// Imprime tela
	imshow("imColor", img);
	imshow("imBorda", borda);
	
	}

	return 0;

}

/*
//Essa funcao pode ser usada para subtrair canais RGB, no exemplo abaixo o canal Red é excluido
img -= Scalar(255, 255, 0);
*/

/*
//Funcao para dividir a imagem em 3 canais
split(img, channel); // Dividir canais
channel[0] = Mat::zeros(img.rows, img.cols, CV_8UC1); // Zerar canal blue
channel[1] = Mat::zeros(img.rows, img.cols, CV_8UC1); // Zerar canal green
channel[2] = Mat::zeros(img.rows, img.cols, CV_8UC1); // Zerar canal red
merge(channel, 3, img); // Mesclar os canais resultantes
*/