#include <iostream>
using namespace std;

#include "olcNoiseMaker.cpp"

atomic<double> dFrequencyOutput = 0.0;
double dOctaveBaseFrequency = 110.0; // A2
double d12thRootOf2 = pow(2.0, 1.0 / 12.0);

double MakeNoise(double dTime)
{
	double dOutput = sin(dFrequencyOutput * 2.0 * 3.14159 * dTime);

	return dOutput * 0.5;
	//if (dOutput > 0)
	//{
	//	return 0.2;
	//}
	//else
	//{
	//	return -0.2;
	//}
}

int main()
{
	// Collect the sound hardware
	vector<wstring> devices = olcNoiseMaker<short>::Enumerate();

	// Display all findings
	for (auto d : devices) wcout << "Found output device." << d << endl;

	// Create sound machine
	olcNoiseMaker<short> sound(devices[0], 44100, 1, 8, 512);

	// Link noise function with sound machine
	sound.SetUserFunction(MakeNoise);

	wcout << endl <<
		"|   |   |   |   |   | |   |   |   |   | |   | |   |   |   |" << endl <<
		"|   | S |   |   | F | | G |   |   | J | | K | | L |   |   |" << endl <<
		"|   |___|   |   |___| |___|   |   |___| |___| |___|   |   |__" << endl <<
		"|     |     |     |     |     |     |     |     |     |     |" << endl <<
		"|  Z  |  X  |  C  |  V  |  B  |  N  |  M  |  ,  |  .  |  /  |" << endl <<
		"|_____|_____|_____|_____|_____|_____|_____|_____|_____|_____|" << endl << endl;

	int nCurrentKey = -1;
	bool bKeyPressed = false;
	while (1)
	{
		bKeyPressed = false;
		for (int k = 0; k < 16; k++)
		{
			if (GetAsyncKeyState((unsigned char)("ZSXCFVGBNJMK\xbcL\xbe\xbf"[k])) & 0x8000)
			{
				if (nCurrentKey != k)
				{
					dFrequencyOutput = dOctaveBaseFrequency * pow(d12thRootOf2, k);
					wcout << "\rNote On : " << sound.GetTime() << "s " << dFrequencyOutput << "Hz";
					nCurrentKey = k;
				}

				bKeyPressed = true;
			}
		}

		if (!bKeyPressed)
		{
			if (nCurrentKey != -1)
			{
				wcout << "\rNote Off: " << sound.GetTime() << "s                        ";
				nCurrentKey = -1;
			}

			dFrequencyOutput = 0.0;
		}
	}

		return 0;
}