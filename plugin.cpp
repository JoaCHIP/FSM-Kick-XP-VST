#include <math.h>
#include <algorithm>
#include "plugin.h"
#include <list>

using namespace std;
class Locker
{
	Lock* l;
public:
	Locker(Lock* l)
	{
		this->l = l;
		this->l->lock();
	};
	~Locker()
	{
		this->l->unlock();
	}
};

// these parameters were all "const", but is that necessary?
SynthParameter paraStartFrq =
{
	"StartFrq",
	"Start frequency",
	"deep", "zappy",
	1,
	240,
	99,
	145,
};

SynthParameter paraEndFrq =
{
	"EndFrq",
	"End frequency",
	"tonal", "sweepy",
	1,
	240,
	50,
	66
};

SynthParameter paraBuzzAmt =
{
	"Buzz",
	"Amount of buzz",
	"soft", "punchy",
	0,
	100,
	0,
	55
};

SynthParameter paraClickAmt =
{
	"Click",
	"Amount of click",
	"muffled", "sharp",
	0,
	100,
	0,
	28
};

SynthParameter paraPunchAmt =
{
	"Punch",
	"Amount of punch",
	"mellow", "hard",
	0,
	100,
	17,
	47
};

SynthParameter paraToneDecay =
{
	"ToneDecR",
	"Tone decay rate",
	"low", "high",
	1,
	240,
	40,
	140,
};

SynthParameter paraToneShape =
{
	"ToneDecS",
	"Tone decay shape",
	"dull", "squeaky",
	1,
	240,
	17,
	27
};

SynthParameter paraBuzzDecayRate =
{
	"BuzzDecR",
	"Buzz decay rate",
	"deep", "tonal",
	1,
	240,
	60,
	150
};

SynthParameter paraClicknPunchDecayRate =
{
	"C+P DecR",
	"Click+Punch decay rate",
	"", "",
	1,
	240,
	55,
	164
};

SynthParameter paraDecaySlope =
{
	"DecSlope",
	"Amplitude decay slope",
	"short", "long",
	1,
	240,
	60,
	150
};

SynthParameter paraDecayTime =
{
	"DecTime",
	"Amplitude decay time",
	"gated", "sustained",
	1,
	240,
	32,
	41
};

SynthParameter paraReleaseSlope =
{
	"RelSlope",
	"Amplitude release slope",
	"short", "long",
	1,
	240,
	70,
	130
};

SynthParameter *synthParameters[] =
{
	&paraStartFrq,
	&paraEndFrq,
	&paraBuzzAmt,
	&paraClickAmt,
	&paraPunchAmt,
	&paraToneDecay,
	&paraToneShape,
	&paraBuzzDecayRate,
	&paraClicknPunchDecayRate,
	&paraDecaySlope,
	&paraDecayTime,
	&paraReleaseSlope,
};


/// <summary>Converts from a normalized (0.00 - 1.00) range to byte range suitable for the parameter.</summary>
/// <param name="floatVal">The float value.</param>
/// <param name="param">The parameter.</param>
float to_range(float floatVal, const SynthParameter &param)
{
	float fByte = (float)param.MinValue + floatVal * (float)(param.MaxValue - param.MinValue);
	return fByte < param.MinValue ? param.MinValue : fByte > param.MaxValue ? param.MaxValue : fByte;
}

/// <summary>Converts from a byte range to a normalized (0.00 - 1.00) range.</summary>
/// <param name="byteVal">The byte value.</param>
/// <param name="param">The parameter.</param>
float from_range(byte byteVal, const SynthParameter &param)
{
	float fByte = byteVal;
	fByte = fByte - param.MinValue;
	fByte = fByte / (float)(param.MaxValue - param.MinValue);
	return fByte < 0.0f ? 0.0f : fByte > 1.0f ? 1.0f : fByte;
}

ParameterDescriptionWord describe(byte byteVal, const SynthParameter &param)
{
	ParameterDescriptionWord result;
	result.Name;
	float ratio = byteVal;
	ratio = ratio - (float)param.DefValueMin;
	ratio = ratio / (float)(param.DefValueMax - param.DefValueMin);
	result.Weight = fabs(ratio - 0.5f);

	if (ratio < 0.1f)
	{
		 sprintf(result.Name, param.DescriptionWhenLow);
		//sprintf(result.Name, "%s(%.0f) ", param.DescriptionWhenLow, ratio*100);
	}
	else if (ratio > 0.9f)
	{
		sprintf(result.Name, param.DescriptionWhenHigh);
		//sprintf(result.Name, "%s(%.0f) ", param.DescriptionWhenHigh, ratio * 100);
	}
	else
	{
		result.Name[0] = 0;
		// sprintf(result.Name, "val(%.0f) ", ratio * 100);
	}

	return result;
}

typedef unsigned long int  u4;
typedef struct ranctx { u4 a; u4 b; u4 c; u4 d; } ranctx;

#define rot(x,k) (((x)<<(k))|((x)>>(32-(k))))
u4 ranval(ranctx *x)
{
	u4 e = x->a - rot(x->b, 27);
	x->a = x->b ^ rot(x->c, 17);
	x->b = x->c + x->d;
	x->c = x->d + e;
	x->d = e + x->a;
	return x->d;
}

void raninit(ranctx *x, u4 seed)
{
	u4 i;
	x->a = 0xf1ea5eed, x->b = x->c = x->d = seed;
	for (i = 0; i < 20; ++i)
	{
		(void)ranval(x);
	}
}
static ranctx rctx;
AudioEffect* createEffectInstance(audioMasterCallback audioMaster)
{
	return new FSM_VST_Plugin(audioMaster);
}

void FSM_VST_Program::set(char* _name, int start, int end,
	int buzz, int click, int punch, int tDecR, int tDecS,
	int bDecR, int CPDecR, int ADecS, int ADecT, int ARelS)
{
	bStartFrq = from_range(start, paraStartFrq);
	bEndFrq = from_range(end, paraEndFrq);
	bBuzzAmt = from_range(buzz, paraBuzzAmt);
	bClickAmt = from_range(click, paraClickAmt);
	bPunchAmt = from_range(punch, paraPunchAmt);
	bToneDecay = from_range(tDecR, paraToneDecay);
	bToneShape = from_range(tDecS, paraToneShape);
	bBDecay = from_range(bDecR, paraBuzzDecayRate);
	bCDecay = from_range(CPDecR, paraClicknPunchDecayRate);
	bDecSlope = from_range(ADecS, paraDecaySlope);
	bDecTime = from_range(ADecT, paraDecayTime);
	bRelSlope = from_range(ARelS, paraReleaseSlope);

	if (_name[0] == 0)
	{
		// if no preset name was provided, invent one
		std::string presetName;
		std::list<ParameterDescriptionWord> words;

		// build a list of adjectives describing the preset
		words.push_back(describe(start, paraStartFrq));
		words.push_back(describe(end, paraEndFrq));
		words.push_back(describe(buzz, paraBuzzAmt));
		words.push_back(describe(click, paraClickAmt));
		words.push_back(describe(punch, paraPunchAmt));
		words.push_back(describe(tDecR, paraToneDecay));
		words.push_back(describe(tDecS, paraToneShape));
		words.push_back(describe(bDecR, paraBuzzDecayRate));
		words.push_back(describe(CPDecR, paraClicknPunchDecayRate));
		words.push_back(describe(ADecS, paraDecaySlope));
		words.push_back(describe(ADecT, paraDecayTime));
		words.push_back(describe(ARelS, paraReleaseSlope));

		bool mustAddSeparator = false;
		words.sort([](const ParameterDescriptionWord &a, const ParameterDescriptionWord &b) { return a.Weight > b.Weight; });
		for each (ParameterDescriptionWord desc in words)
		{
			int descriptionLength = strlen(desc.Name);
			if (descriptionLength > 0 && presetName.length() + descriptionLength < kVstMaxProgNameLen-1)
			{
				if (mustAddSeparator)
				{
					presetName.append(" ");
				}

				presetName.append(desc.Name);
				mustAddSeparator = true;
			}
		}

		presetName[0] = toupper(presetName[0]);
		vst_strncpy(this->name, presetName.c_str(), kVstMaxProgNameLen);
	}
	else
	{
		vst_strncpy(this->name, _name, kVstMaxProgNameLen);
	}
}

#define RND(x) (ranval(&rctx)%x)

#define RNDR(x,y) ((ranval(&rctx)%(y-x)) + x)

FSM_VST_Plugin::FSM_VST_Plugin(audioMasterCallback audioMaster)
	: AudioEffectX(audioMaster, kNumPrograms, kNumParams)
{
	raninit(&rctx, 238947);
	lock = new Lock();
	programs = new FSM_VST_Program[kNumPrograms];
	fVolume = 0.8f;
	issetprogram = false;

	// initialize programs
	programs[0].set("Init", 145, 50, 55, 28, 47,  70, 27, 110,  55,  100, 32, 110);
	programs[1].set("Soft", 99,  66,  0,  0, 17, 104, 17, 69, 164, 94, 41, 140);
	for (int i = 2; i < kNumPrograms; i++)
	{
		char presetname[kVstMaxProgNameLen];
		presetname[0] = 0;
		programs[i].set(presetname,
			RNDR(70, 160),
			RNDR(40, 90),
			RNDR(0, 80),
			RNDR(0, 40),
			RNDR(10, 60),
			RNDR(10, 130),
			RNDR(10, 35),
			RNDR(50, 80),
			RNDR(40, 200),
			RNDR(20, 140),
			RNDR(20, 60),
			RNDR(100, 150));
	}

	setProgram(0);

	if (audioMaster)
	{
		setNumInputs(0);				// no inputs
		setNumOutputs(kNumOutputs);		// 2 outputs, 1 for each oscillator
		canProcessReplacing();
		isSynth();
		setUniqueID('FSMX');
	}

	initProcess();
	suspend();
}

void FSM_VST_Plugin::initProcess()
{
#ifdef DEBUG_CONSOLE
	initDebug();
#endif //DEBUG_CONSOLE
	for (int i = 0; i < 1024; i++)
	{
		thumpdata1[i] = float(sin(1.37*i + 0.1337*(1024 - i)*sin(1.1*i))*pow(1.0 / 256.0, i / 1024.0));
	}
}

#ifdef DEBUG_CONSOLE
VstIntPtr FSM_VST_Plugin::dispatcher(VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
{
	debugDispatcher(opcode, index, value, ptr, opt);
	return AudioEffectX::dispatcher(opcode, index, value, ptr, opt);
}
#endif

FSM_VST_Plugin::~FSM_VST_Plugin()
{
	if (lock)
	{
		delete lock;
	}

	if (programs)
	{
		delete[] programs;
	}
#ifdef DEBUG_CONSOLE
	FreeConsole();
#endif;
}

void FSM_VST_Plugin::setProgram(VstInt32 program)
{
	if (program < 0 || program >= kNumPrograms)
	{
		return;
	}

	lock->lock();
	allNotesOff(false);
	curProgram = program;
	lock->unlock();
}

void FSM_VST_Plugin::setProgramName(char* name)
{
	//if (name != NULL && programs != NULL  && curProgram >= 0 && curProgram < kNumPrograms && programs[curProgram].name != NULL)
	//	vst_strncpy (programs[curProgram].name, name, kVstMaxProgNameLen);
	//if (name != NULL) {

	//	//dprintf("setProgramName %s\n", name);
	//}
}

void FSM_VST_Plugin::getProgramName(char* name)
{
	if (name != NULL && programs != NULL && curProgram >= 0 && programs[curProgram].name != NULL)
	{
		vst_strncpy(name, programs[curProgram].name, kVstMaxProgNameLen);
	}
}

void FSM_VST_Plugin::getParameterLabel(VstInt32 index, char* label)
{
	vst_strncpy(label, "", kVstMaxParamStrLen);
}

void FSM_VST_Plugin::getParameterDisplay(VstInt32 index, char* text)
{
	text[0] = 0;
	switch (index)
	{
	case kVolume:		float2string(100.0f * fVolume, text, kVstMaxParamStrLen); break;
	case kStartFrq:		float2string(to_range(current()->bStartFrq, paraStartFrq), text, kVstMaxParamStrLen); break;
	case kEndFrq:		float2string(to_range(current()->bEndFrq, paraEndFrq), text, kVstMaxParamStrLen); break;
	case kBuzzAmt:		float2string(to_range(current()->bBuzzAmt, paraBuzzAmt), text, kVstMaxParamStrLen);  break;
	case kClickAmt:		float2string(to_range(current()->bClickAmt, paraClickAmt), text, kVstMaxParamStrLen); break;
	case kPunchAmt:		float2string(to_range(current()->bPunchAmt, paraPunchAmt), text, kVstMaxParamStrLen); break;
	case kToneDecay:	float2string(to_range(current()->bToneDecay, paraToneDecay), text, kVstMaxParamStrLen); break;
	case kToneShape:	float2string(to_range(current()->bToneShape, paraToneShape), text, kVstMaxParamStrLen); break;
	case kBDecay:		float2string(to_range(current()->bBDecay, paraBuzzDecayRate), text, kVstMaxParamStrLen); break;
	case kCDecay:		float2string(to_range(current()->bCDecay, paraClicknPunchDecayRate), text, kVstMaxParamStrLen); break;
	case kDecSlope:		float2string(to_range(current()->bDecSlope, paraDecaySlope), text, kVstMaxParamStrLen); break;
	case kDecTime:		float2string(to_range(current()->bDecTime, paraDecayTime), text, kVstMaxParamStrLen);  break;
	case kRelSlope:		float2string(to_range(current()->bRelSlope, paraReleaseSlope), text, kVstMaxParamStrLen); break;
	}
}

void FSM_VST_Plugin::getParameterName(VstInt32 index, char* label)
{
	if (index == kVolume)
	{
		vst_strncpy(label, "Volume", kVstMaxParamStrLen);
	}
	else if (index > 0 && index < kNumParams)
	{
		const SynthParameter* param = synthParameters[index - 1];
		vst_strncpy(label, param->Name, kVstMaxParamStrLen);
	}
}

void FSM_VST_Plugin::setParameter(VstInt32 index, float value)
{
	FSM_VST_Program *ap = &programs[curProgram];
	switch (index)
	{
	case kVolume: fVolume = value; break;
	case kStartFrq: current()->bStartFrq = value; break;
	case kEndFrq: current()->bEndFrq = value; break;
	case kBuzzAmt: current()->bBuzzAmt = value; break;
	case kClickAmt: current()->bClickAmt = value; break;
	case kPunchAmt: current()->bPunchAmt = value; break;
	case kToneDecay: current()->bToneDecay = value; break;
	case kToneShape: current()->bToneShape = value; break;
	case kBDecay: current()->bBDecay = value; break;
	case kCDecay: current()->bCDecay = value; break;
	case kDecSlope: current()->bDecSlope = value; break;
	case kDecTime: current()->bDecTime = value; break;
	case kRelSlope: current()->bRelSlope = value; break;
	}
}

float FSM_VST_Plugin::getParameter(VstInt32 index)
{
	float value = 0;
	switch (index)
	{
	case kVolume: value = fVolume; break;
	case kStartFrq: value = current()->bStartFrq; break;
	case kEndFrq: value = current()->bEndFrq; break;
	case kBuzzAmt: value = current()->bBuzzAmt; break;
	case kClickAmt: value = current()->bClickAmt; break;
	case kPunchAmt: value = current()->bPunchAmt; break;
	case kToneDecay: value = current()->bToneDecay; break;
	case kToneShape: value = current()->bToneShape; break;
	case kBDecay: value = current()->bBDecay; break;
	case kCDecay: value = current()->bCDecay; break;
	case kDecSlope: value = current()->bDecSlope; break;
	case kDecTime: value = current()->bDecTime; break;
	case kRelSlope: value = current()->bRelSlope; break;
	}
	return value;
}

void FSM_Voice::setParameters(ProgramParameters *ptval, float srate)
{
	this->StartFrq = (float)(33.0*pow(128, to_range(ptval->bStartFrq, paraStartFrq) / 240.0));
	this->EndFrq = (float)(33.0*pow(16, to_range(ptval->bEndFrq, paraEndFrq) / 240.0));
	this->BuzzAmt = 3 * (float)(to_range(ptval->bBuzzAmt, paraBuzzAmt) / 100.0);
	this->ClickAmt = (float)(to_range(ptval->bClickAmt, paraClickAmt) / 100.0);
	this->PunchAmt = (float)(to_range(ptval->bPunchAmt, paraPunchAmt) / 100.0);
	this->ToneDecay = (float)((to_range(1-sqrtf(ptval->bToneDecay), paraToneDecay) / 240.0)*(1.0 / 400.0)*(44100.0 / srate));
	this->ToneShape = (float)(to_range(ptval->bToneShape, paraToneShape) / 240.0);
	this->BuzzDecay = (float)(to_range(1-sqrtf(ptval->bBDecay), paraBuzzDecayRate) / 240.0);
	this->ClicknPunchDecay = (float)(to_range(ptval->bCDecay, paraClicknPunchDecayRate) / 2240.0);
	this->AmplitudeDecaySlope = (float)pow(20, to_range(1-sqrtf(ptval->bDecSlope), paraDecaySlope) / 240.0 - 1) * 25 / srate;
	this->AmplitudeDecayTime = (float)(to_range(ptval->bDecTime, paraDecayTime)*srate / 240.0);
	this->ReleaseSlope = (float)pow(20, to_range(1- sqrtf(ptval->bRelSlope), paraReleaseSlope) / 240.0 - 1) * 25 / srate;

	if (this->currentNote != NOTE_OFF)
	{
		int v = this->currentNote - 24;
		this->PitchLimit = (float)(440.0*pow(2, (v - 69) / 12.0));
	}
}

bool FSM_VST_Plugin::getOutputProperties(VstInt32 index, VstPinProperties* properties)
{
	if (index == 0 || index == 1)
	{
		properties->flags = kVstPinIsActive | kVstPinIsStereo;
	}
	if (index == 0)
	{
		strcpy(properties->label, "Left output");
		strcpy(properties->shortLabel, "L out");
		return true;
	}
	else if (index == 1)
	{
		strcpy(properties->label, "Right output");
		strcpy(properties->shortLabel, "R out");
		return true;
	}
	return false;
}

bool FSM_VST_Plugin::getProgramNameIndexed(VstInt32 category, VstInt32 index, char* text)
{
	if (index >= 0 && index < kNumPrograms)
	{
		vst_strncpy(text, programs[index].name, kVstMaxProgNameLen);
		return true;
	}
	return false;
}

bool FSM_VST_Plugin::getEffectName(char* name)
{
	vst_strncpy(name, "DrumSynth", kVstMaxEffectNameLen);
	return true;
}

bool FSM_VST_Plugin::getVendorString(char* text)
{
	vst_strncpy(text, PLUGIN_VENDOR_NAME, kVstMaxVendorStrLen);
	return true;
}

bool FSM_VST_Plugin::getProductString(char* text)
{
	vst_strncpy(text, PLUGIN_PRODUCT_NAME, kVstMaxProductStrLen);
	return true;
}

VstInt32 FSM_VST_Plugin::getVendorVersion()
{
	return 1000;
}

VstInt32 FSM_VST_Plugin::canDo(char* text)
{
	//dprintf("canDo %s\n", text);
	if (!strcmp(text, "receiveVstEvents"))
		return 1;
	if (!strcmp(text, "receiveVstMidiEvent"))
		return 1;
	if (!strcmp(text, "receiveVstTimeInfo"))
		return 1;
	return -1;		// explicitly can't do; 0 => don't know
}

VstInt32 FSM_VST_Plugin::getNumMidiInputChannels()
{
	return 1;
}

VstInt32 FSM_VST_Plugin::getNumMidiOutputChannels()
{
	return 0;		// no MIDI output back to Host app
}

void FSM_VST_Plugin::setSampleRate(float sampleRate)
{
	lock->lock();
	allNotesOff(true);
	AudioEffectX::setSampleRate(sampleRate);
	lock->unlock();
}

void FSM_VST_Plugin::setBlockSize(VstInt32 blockSize)
{
	lock->lock();
	allNotesOff(true);
	AudioEffectX::setBlockSize(blockSize);
	lock->unlock();
}


#define ENTER_LOCK \
	if (!locked) { \
		locked++; \
		lock->lock(); \
	}

VstInt32 FSM_VST_Plugin::processEvents(VstEvents* ev)
{
	int locked = 0;
	for (VstInt32 i = 0; i < ev->numEvents; i++)
	{
		if ((ev->events[i])->type != kVstMidiType)
			continue;

		VstMidiEvent* event = (VstMidiEvent*)ev->events[i];
		char* midiData = event->midiData;
		VstInt32 status = midiData[0] & 0xF0;	// ignoring channel
		if (status == 0x90 || status == 0x80)	// we only look at notes
		{
			VstInt32 note = midiData[1] & 0x7F;
			VstInt32 velocity = midiData[2] & 0x7F;
			if (status == 0x80)
			{
				velocity = 0;					// note off by velocity 0
			}

			if (!velocity)
			{
				ENTER_LOCK;
				noteOff(note, event->deltaFrames);
			}
			else
			{
				ENTER_LOCK;
				noteOn(note, velocity, event->deltaFrames);
			}
		}
		else if (status == 0xB0)
		{
			if (midiData[1] == 0x7E || midiData[1] == 0x7B)
			{	// all notes off
				ENTER_LOCK;
				allNotesOff(true);
			}
		}
		event++;
	}
	if (locked)
	{
		lock->unlock();
	}
	return 1;
}

void FSM_VST_Plugin::noteOff(VstInt32 note, VstInt32 deltaFrames)
{
	std::vector<FSM_Voice*>::iterator it = voices.begin();
	while (it != voices.end())
	{
		FSM_Voice *voice = *it;
		if (voice->currentNote == note)
		{
			voice->setNoteReleaseDeta(deltaFrames);
		}
		it++;
	}
}

void FSM_VST_Plugin::noteOn(VstInt32 note, VstInt32 velocity, VstInt32 delta)
{
	if (issetprogram)
		return;
	FSM_Voice* voice;
	voice = new FSM_Voice(note, velocity, delta);
	voice->setParameters(current(), sampleRate);
	voice->trigger();
	voices.push_back(voice);
}

void FSM_VST_Plugin::allNotesOff(bool decay)
{
	std::vector<FSM_Voice*>::iterator it = voices.begin();
	while (it != voices.end())
	{
		FSM_Voice *_note = *it;
		_note->setState(decay ? VOICE_DECAY : VOICE_KILLED);
		it++;
	}
}

#define INTERPOLATE(pos,start,end) ((start)+(pos)*((end)-(start)))

bool FSM_VST_Plugin::processVoice(FSM_Voice *trk, float *pout, int c, float gain)
{
	trk->OscPhase = fmod(trk->OscPhase, 1.0);
	float ratio = trk->ThisEndFrq / trk->ThisStartFrq;
	int i = 0;
	double xSin = trk->xSin, xCos = trk->xCos;
	double dxSin = trk->dxSin, dxCos = trk->dxCos;
	float l_value = 0;
	float amplitude = trk->Amp;
	float decay_amplitude = trk->DecAmp;
	float b_amp = trk->BAmp;
	float mul_amp = trk->MulBAmp;
	float c_amp = trk->CAmp;
	float mul_c_amp = trk->MulCAmp;
	float volume = 0.5f*trk->ThisCurVolume*gain;
	bool amphigh = amplitude >= 16;
	int age = trk->Age;
	float sr = this->getSampleRate();
	float odsr = 1.0f / sr;
	int proc = 0;
	int loop = 0;

	while (i < c)
	{
		if (trk->LeftOver <= 0)
		{
			trk->LeftOver = 32;
			double EnvPoint = trk->EnvPhase*trk->ThisToneDecay;
			double ShapedPoint = pow(EnvPoint, trk->ThisToneShape*2.0);
			trk->Frequency = (float)(trk->ThisStartFrq*pow((double)ratio, ShapedPoint));
			if (trk->Frequency > 10000.f) trk->EnvPhase = 6553600;
			if (trk->EnvPhase < trk->ThisAmplitudeDecayTime)
			{
				trk->DecAmp = decay_amplitude = trk->ThisAmplitudeDecaySlope;
				trk->Amp = amplitude = (float)(1 - decay_amplitude * trk->EnvPhase);
			}
			else
			{
				decay_amplitude = trk->ThisAmplitudeDecaySlope;
				amplitude = (float)(1 - decay_amplitude * trk->ThisAmplitudeDecayTime);
				if (amplitude > 0)
				{
					trk->DecAmp = decay_amplitude = trk->ThisReleaseSlope;
					trk->Amp = amplitude = amplitude - decay_amplitude * (trk->EnvPhase - trk->ThisAmplitudeDecayTime);
				}
			}

			if (trk->Amp <= 0)
			{
				trk->Amp = 0;
				trk->DecAmp = 0;
				return amphigh;
			}

			trk->BAmp = b_amp = trk->BuzzAmt*(float)(pow(1.0f / 256.0f, trk->ThisBuzzDecay*trk->EnvPhase*(odsr * 10)));
			float CVal = (float)(pow(1.0f / 256.0f, trk->ThisClicknPunchDecay*trk->EnvPhase*(odsr * 20)));
			trk->CAmp = c_amp = trk->ClickAmt*CVal;
			trk->Frequency *= (1 + 2 * trk->PunchAmt*CVal*CVal*CVal);
			if (trk->Frequency > 10000) trk->Frequency = 10000;
			if (trk->Frequency < trk->ThisPitchLimit) trk->Frequency = trk->ThisPitchLimit;

			trk->MulBAmp = mul_amp = (float)pow(1.0f / 256.0f, trk->ThisBuzzDecay*(10 * odsr));
			trk->MulCAmp = mul_c_amp = (float)pow(1.0f / 256.0f, trk->ThisClicknPunchDecay*(10 * odsr));
			xSin = (float)sin(2.0*3.141592665*trk->OscPhase);
			xCos = (float)cos(2.0*3.141592665*trk->OscPhase);
			dxSin = (float)sin(2.0*3.141592665*trk->Frequency / sr);
			dxCos = (float)cos(2.0*3.141592665*trk->Frequency / sr);
			l_value = 0.0;
			trk->dxSin = dxSin, trk->dxCos = dxCos;
		}
		int max = min(i + trk->LeftOver, c);

		if (amplitude > 0.00001f && volume > 0)
		{
			proc++;
			amphigh = true;
			float old_amplitude = amplitude;
			if (b_amp > 0.01f)
			{
				for (int j = i; j < max; j++)
				{
					pout[j] += float(l_value = float(amplitude*volume*xSin));
					if (xSin > 0)
					{
						float D = (float)(amplitude*volume*b_amp*xSin*xCos);
						pout[j] -= D;
						l_value -= D;
					}
					double xSin2 = double(xSin*dxCos + xCos * dxSin);
					double xCos2 = double(xCos*dxCos - xSin * dxSin);
					xSin = xSin2; xCos = xCos2;
					amplitude -= decay_amplitude;
					b_amp *= mul_amp;
				}
			}
			else
			{
				for (int j = i; j < max; j++)
				{
					pout[j] += float(l_value = float(amplitude*volume*xSin));
					double xSin2 = double(xSin*dxCos + xCos * dxSin);
					double xCos2 = double(xCos*dxCos - xSin * dxSin);
					xSin = xSin2; xCos = xCos2;
					amplitude -= decay_amplitude;
				}
			}

			if (old_amplitude > 0.1f && c_amp > 0.001f)
			{
				int max2 = i + min(max - i, 1024 - age);
				float LVal2 = 0.f;
				for (int j = i; j < max2; j++)
				{
					pout[j] += (LVal2 = old_amplitude * volume*c_amp*this->thumpdata1[age]);
					old_amplitude -= decay_amplitude;
					c_amp *= mul_c_amp;
					age++;
				}
				l_value += LVal2;
			}
		}

		if (amplitude)
		{
			trk->OscPhase += (max - i)*trk->Frequency / sr;
			trk->EnvPhase += max - i;
			trk->LeftOver -= max - i;
		}
		else
		{
			trk->LeftOver = 32000;
		}
		i = max;
	}

	trk->xSin = xSin, trk->xCos = xCos;
	trk->Amp = amplitude;
	trk->BAmp = b_amp;
	trk->CAmp = c_amp;
	trk->Age = age;
	return amphigh;
}

void FSM_VST_Plugin::processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames)
{
	memset(outputs[0], 0, sampleFrames * sizeof(float));
	memset(outputs[1], 0, sampleFrames * sizeof(float));
	if (issetprogram)
	{
		return;
	}

	float gain = powf(fVolume*SCALE_GAIN_OVERHEAD, 1.F / LOG_SCALE_GAIN);
	Locker l(lock);
	if (this->voices.empty())
	{
		return;
	}

	// getVstVersion();		// ToDo: why is this called here?

	for (std::vector<FSM_Voice*>::iterator it = this->voices.begin(); it != this->voices.end(); )
	{
		FSM_Voice* voice = *it;
		if (voice->state >= VOICE_KILLED)
		{
			delete voice;
			it = voices.erase(it);
			continue;
		}

		float* out1 = outputs[0];
		int samples = sampleFrames;
		if (voice->releaseDelta >= sampleFrames)
		{	// release in a future block
			voice->releaseDelta -= sampleFrames;
			if (voice->releaseDelta < 0)
			{
				voice->releaseDelta = 0;
			}
		}

		if (voice->currentDelta >= sampleFrames)	// future
		{
			voice->currentDelta -= sampleFrames;
			++it;
			continue;
		}

		if (voice->currentDelta > 0)
		{
			out1 += voice->currentDelta;
			samples -= voice->currentDelta;
			voice->currentDelta = 0;
		}

		if (samples > 0)
		{
			bool amplitudeHigh = this->processVoice(voice, out1, samples, gain);
			if (voice->releaseDelta < sampleFrames)
			{
				voice->setState(VOICE_DECAY); // decay state not used at all
			}

			if (voice->state >= VOICE_KILLED || !amplitudeHigh)
			{	// voices are removed when amplitude is really low or it voice was killed (no decay)
				delete voice;
				it = voices.erase(it);
				continue;
			}
		}
		++it;
	}
	memcpy(outputs[1], outputs[0], sizeof(float)*sampleFrames);
}

FSM_Voice::FSM_Voice(VstInt32 note, VstInt32 velocity, VstInt32 delta)
{
	currentNote = note;
	currentDelta = delta;
	currentVelocity = velocity;
	releaseDelta = -1;
	state = 0;
	LeftOver = 0;
	EnvPhase = 0;
	OscPhase = 0;
	CurVolume = 0;
	Age = 0;
	Amp = 0;
	DecAmp = 0;
	BAmp = 0;
	MulBAmp = 0;
	CAmp = 0;
	MulCAmp = 0;
	Frequency = 0;
	xSin = 0;
	xCos = 0;
	dxSin = 0;
	dxCos = 0;
}

void FSM_Voice::trigger()
{
	this->state = VOICE_HOLD;
	this->EnvPhase = 0;
	this->OscPhase = this->ClickAmt;
	this->LeftOver = 0;
	this->Age = 0;
	this->Amp = 32;
	this->CurVolume = this->velocity();
	this->ThisPitchLimit = this->PitchLimit;
	this->ThisAmplitudeDecayTime = this->AmplitudeDecayTime;
	this->ThisAmplitudeDecaySlope = this->AmplitudeDecaySlope;
	this->ThisReleaseSlope = this->ReleaseSlope;
	this->ThisBuzzDecay = this->BuzzDecay;
	this->ThisClicknPunchDecay = this->ClicknPunchDecay;
	this->ThisToneDecay = this->ToneDecay;
	this->ThisToneShape = this->ToneShape;
	this->ThisStartFrq = this->StartFrq;
	this->ThisEndFrq = this->EndFrq;
	this->ThisCurVolume = this->CurVolume;
}

void FSM_Voice::setNoteReleaseDeta(VstInt32 delta)
{
	if (releaseDelta < 0 || releaseDelta > delta)
	{
		releaseDelta = delta;
	}
}

void FSM_Voice::setState(int newState)
{
	this->state = max(this->state, newState);
}

FSM_VST_Program::FSM_VST_Program()
{
	vst_strncpy(name, "Init", kVstMaxProgNameLen);

	bStartFrq = from_range(paraStartFrq.DefValue(), paraStartFrq);
	bEndFrq = from_range(paraEndFrq.DefValue(), paraEndFrq);
	bBuzzAmt = from_range(paraBuzzAmt.DefValue(), paraBuzzAmt);
	bClickAmt = from_range(paraClickAmt.DefValue(), paraClickAmt);
	bPunchAmt = from_range(paraPunchAmt.DefValue(), paraPunchAmt);
	bToneDecay = from_range(paraToneDecay.DefValue(), paraToneDecay);
	bToneShape = from_range(paraToneShape.DefValue(), paraToneShape);
	bBDecay = from_range(paraBuzzDecayRate.DefValue(), paraBuzzDecayRate);
	bCDecay = from_range(paraClicknPunchDecayRate.DefValue(), paraClicknPunchDecayRate);
	bDecSlope = from_range(paraDecaySlope.DefValue(), paraDecaySlope);
	bDecTime = from_range(paraDecayTime.DefValue(), paraDecayTime);
	bRelSlope = from_range(paraReleaseSlope.DefValue(), paraReleaseSlope);
}
