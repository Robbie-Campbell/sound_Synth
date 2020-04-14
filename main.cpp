#include <list>
#include <iostream>
#include <algorithm>
using namespace std;

#include "olcNoiseMaker.cpp"

atomic<double> dFrequencyOutput = 0.0;


// The equation to work out Hertz
double w(double dHertz)
{
	return dHertz * 2.0 * PI;
}

//oscilators
#define OSC_SINE 0
#define OSC_SQUARE 1
#define OSC_TRIANGLE 2
#define OSC_SAW_ANA 3
#define OSC_SAW_DIG 4
#define OSC_NOISE 5

// Different audio types using different equations
double osc(double dHertz, double dTime, int nType = OSC_SINE, double dLFOHertz= 0.0, double dLFOAmplitude = 0.0)
{

	double dFreq = w(dHertz) * dTime + dLFOAmplitude * dHertz * sin(w(dLFOHertz) * dTime);
	switch (nType)
	{
	// A sine wave
	case OSC_SINE:
		return sin(dFreq);

	// A square wave
	case OSC_SQUARE:
		return sin(dFreq) > 0.0 ? 0.3 : -0.3;

	// A triangle wave
	case OSC_TRIANGLE:
		return asin(sin(dFreq)) * (2.0 / PI);

	// A mathematically correct saw wave
	case OSC_SAW_ANA:
	{
		double dOutput = 0;
		for (double n = 1.0; n < 50.0; n++)
		{
			dOutput += (sin(n * dFreq)) / n;
		}
	}

	// An estimated saw wave for computational efficiency
	case OSC_SAW_DIG:
		return (2.0 / PI) * (dHertz * PI * fmod(dTime, 1.0 / dHertz) - (PI / 2));

	// A random feedback loop
	case OSC_NOISE:
		return 2.0 * ((double)rand() / (double)RAND_MAX) - 1.0;
	default: 
		return 0;
	}
}


// An envelope which is able to make the music flow more naturally with decreasing sound over time which stabilised and eases out, closer to a real instrument
struct sEnvelopeADSR
{

	// Define the initial pointers
	double dAttackTime;
	double dDecayTime;
	double dReleaseTime;

	double dSustainAmplitude;
	double dStartAmplitude;

	double dTriggerOnTime;
	double dTriggerOffTime;

	bool bNoteOn;
	sEnvelopeADSR() 
	{

		// Initialise the pointers
		dAttackTime = 0.01;
		dDecayTime = 1.0;
		dReleaseTime = 1.0;

		dSustainAmplitude = 0.0;
		dStartAmplitude = 1.0;

		dTriggerOnTime = 0.0;
		dTriggerOffTime = 0.0;
		bNoteOn = false;
	}

	// The function getamplitude keeps track of how long the note has been held for
	double GetAmplitude(double dTime)
	{
		double dAmplitude = 0.0;

		double dLifeTime = dTime - dTriggerOnTime;

		if (bNoteOn)
		{
			// Attack
			if (dLifeTime <= dAttackTime)
				dAmplitude = (dLifeTime / dAttackTime) * dStartAmplitude;

			// decay
			if (dLifeTime > dAttackTime && dLifeTime <= (dAttackTime + dDecayTime))
				dAmplitude = ((dLifeTime - dAttackTime) / dDecayTime) * (dSustainAmplitude - dStartAmplitude) + dStartAmplitude;

			// sustain 
			if (dLifeTime > (dAttackTime + dDecayTime))
			{
				dAmplitude = dSustainAmplitude;
			}
		}
		else
		{
			//Release
			dAmplitude = ((dTime - dTriggerOffTime) / dReleaseTime) * (0.0 - dSustainAmplitude) + dSustainAmplitude;
		}
		if (dAmplitude <= 0.0001)
		{
			dAmplitude = 0;
		}

		return dAmplitude;
	}

	// The function for when a key is pressed
	void NoteOn(double dTimeOn)
	{
		dTriggerOnTime = dTimeOn;
		bNoteOn = true;
	}

	// The function for when a key is released
	void NoteOff(double dTimeOff)
	{
		dTriggerOffTime = dTimeOff;
		bNoteOn = false;
	}
};

// The variables 

struct instrument
{
	double dVolume;
	sEnvelopeADSR env;

	virtual double sound(double dTime, double dFrequency) = 0;
};



struct bell : public instrument
{
	bell()
	{
		env.dAttackTime = 0.01;
		env.dDecayTime = 1.0;
		env.dSustainAmplitude = 0.0;
		env.dReleaseTime = 1.0;

		dVolume = 1.0;

	}
	double sound(double dTime, double dFrequency)
	{
		double dOutput = env.GetAmplitude(dTime) *
			(
				+1.0 * osc(dFrequencyOutput * 2.0, dTime, OSC_SINE, 5.0, 0.001)
				+ 0.5 * osc(dFrequencyOutput * 4.0, dTime, OSC_SINE)
				+ 0.25 * osc(dFrequencyOutput * 6.0, dTime, OSC_SINE)
			);
		return dOutput;
	}
};


struct harmonica : public instrument
{
	harmonica()
	{
		env.dAttackTime = 0.01;
		env.dDecayTime = 0.2;
		env.dSustainAmplitude = 0.9;
		env.dReleaseTime = 1.0;

		dVolume = 1.0;

	}
	double sound(double dTime, double dFrequency)
	{
		double dOutput = env.GetAmplitude(dTime) *
			(
				+1.0 * osc(dFrequencyOutput, dTime, OSC_SQUARE, 5.0, 0.001)
				+ 0.5 * osc(dFrequencyOutput * 1.5, dTime, OSC_SQUARE)
				+ 0.25 * osc(dFrequencyOutput * 2.0, dTime, OSC_SQUARE)
				+ 0.005 * osc(0, dTime, OSC_NOISE)
			);
		return dOutput;
	}
};
double dOctaveBaseFrequency = 110.0; // A2
double d12thRootOf2 = pow(2.0, 1.0 / 12.0);
sEnvelopeADSR envelope;



instrument *voice = nullptr;

double MakeNoise(double dTime)
{
	double dOutput = voice->sound(dTime, dFrequencyOutput);
	return dOutput * 0.4;
}

int main()
{
	// Collect the sound hardware
	vector<wstring> devices = olcNoiseMaker<short>::Enumerate();

	// Display all findings
	for (auto d : devices) wcout << "Found output device." << d << endl;

	// Create sound machine
	olcNoiseMaker<short> sound(devices[0], 44100, 1, 8, 512);
	voice = new harmonica();

	// Link noise function with sound machine
	sound.SetUserFunction(MakeNoise);

	// Draw a terminal keyboard
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
					voice->env.NoteOn(sound.GetTime());
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
				voice->env.NoteOff(sound.GetTime());
			}
		}
	}

		return 0;
}