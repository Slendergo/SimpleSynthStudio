#include "pch.h"
#include "Audio/SynthLibrary.h"

namespace SimpleSynthStudio
{
	namespace
	{
		using Layers = std::vector<SynthLayer>;

		SynthVoice Tone(Waveform wave, float startHz, float endHz, float hold, float release, float volume, float attack = 0.002f, float decay = 0.04f, float sustain = 0.5f)
		{
			SynthVoice voice;
			voice.Wave = wave;
			voice.Frequency = startHz;
			voice.EndFrequency = endHz;
			voice.Duration = hold;
			voice.Release = release;
			voice.Volume = volume;
			voice.Attack = attack;
			voice.Decay = decay;
			voice.Sustain = sustain;
			return voice;
		}

		SynthVoice Burst(float cutoff, float endCutoff, float hold, float release, float volume, float attack = 0.001f, float sustain = 0.5f)
		{
			SynthVoice voice = Tone(Waveform::Noise, 440.0f, 440.0f, hold, release, volume, attack, 0.04f, sustain);
			voice.Cutoff = cutoff;
			voice.EndCutoff = endCutoff;
			return voice;
		}

		SynthVoice Thump(float startHz, float endHz, float release, float volume)
		{
			return Tone(Waveform::Sine, startHz, endHz, 0.01f, release, volume, 0.001f, 0.02f, 0.5f);
		}

		SynthVoice Lowpass(SynthVoice voice, float cutoff, float endCutoff = 0.0f)
		{
			voice.Cutoff = cutoff;
			voice.EndCutoff = endCutoff;
			return voice;
		}

		SynthVoice Wobble(SynthVoice voice, float rate, float depth)
		{
			voice.VibratoRate = rate;
			voice.VibratoDepth = depth;
			return voice;
		}

		SynthVoice Pulse(SynthVoice voice, float width)
		{
			voice.PulseWidth = width;
			return voice;
		}

		SynthLayer L(const SynthVoice& voice, float start = 0.0f)
		{
			return { voice, start };
		}

		void AddNotes(Layers& layers, Waveform wave, std::initializer_list<float> frequencies, float start, float spacing, float hold, float release, float volume, float finalHold)
		{
			std::size_t index = 0;
			for (const float frequency : frequencies) {
				const bool last = index + 1 == frequencies.size();
				const SynthVoice voice = Tone(wave, frequency, frequency, last ? finalHold : hold, release, volume, 0.004f, 0.03f, 0.8f);
				layers.push_back(L(voice, start + static_cast<float>(index) * spacing));
				index++;
			}
		}

		void Add(std::vector<SynthEffect>& effects, const char* category, const char* name, Layers layers)
		{
			effects.push_back({ name, category, std::move(layers) });
		}

		void AddGuns(std::vector<SynthEffect>& effects)
		{
			Add(effects, "Guns", "Pistol", { L(Burst(6000.0f, 1500.0f, 0.015f, 0.12f, 0.6f)), L(Thump(220.0f, 60.0f, 0.1f, 0.7f)) });
			Add(effects, "Guns", "Revolver", { L(Burst(5000.0f, 1000.0f, 0.02f, 0.2f, 0.7f)), L(Thump(160.0f, 45.0f, 0.18f, 0.8f)) });
			Add(effects, "Guns", "Rifle", { L(Burst(5000.0f, 1200.0f, 0.02f, 0.18f, 0.6f)), L(Thump(180.0f, 45.0f, 0.15f, 0.7f)), L(Burst(800.0f, 400.0f, 0.02f, 0.25f, 0.2f), 0.02f) });
			Add(effects, "Guns", "Shotgun", { L(Burst(7000.0f, 600.0f, 0.05f, 0.3f, 0.8f)), L(Thump(120.0f, 35.0f, 0.25f, 0.8f)), L(Burst(2000.0f, 500.0f, 0.03f, 0.35f, 0.4f), 0.02f) });
			Add(effects, "Guns", "Sniper", { L(Burst(4000.0f, 400.0f, 0.03f, 0.5f, 0.7f)), L(Thump(100.0f, 30.0f, 0.4f, 0.8f)), L(Burst(500.0f, 300.0f, 0.05f, 0.6f, 0.15f), 0.12f) });
			Add(effects, "Guns", "Silenced Pistol", { L(Burst(2500.0f, 800.0f, 0.01f, 0.08f, 0.35f)), L(Thump(300.0f, 100.0f, 0.05f, 0.2f)) });
			Add(effects, "Guns", "Reload", { L(Tone(Waveform::Square, 1800.0f, 1400.0f, 0.005f, 0.03f, 0.3f)), L(Tone(Waveform::Square, 500.0f, 800.0f, 0.04f, 0.04f, 0.2f), 0.18f), L(Burst(3000.0f, 1500.0f, 0.005f, 0.05f, 0.4f), 0.32f), L(Thump(200.0f, 100.0f, 0.04f, 0.4f), 0.32f) });
			Add(effects, "Guns", "Empty Click", { L(Tone(Waveform::Square, 1200.0f, 800.0f, 0.004f, 0.03f, 0.3f)) });
			Add(effects, "Guns", "Plasma Gun", { L(Lowpass(Tone(Waveform::Saw, 900.0f, 150.0f, 0.1f, 0.08f, 0.4f), 4000.0f, 800.0f)), L(Tone(Waveform::Sine, 300.0f, 80.0f, 0.08f, 0.08f, 0.4f)) });
			Add(effects, "Guns", "Laser Blaster", { L(Tone(Waveform::Square, 1400.0f, 200.0f, 0.18f, 0.05f, 0.3f)), L(Lowpass(Tone(Waveform::Saw, 1400.0f, 200.0f, 0.18f, 0.05f, 0.12f), 3000.0f, 600.0f)) });
			Add(effects, "Guns", "Charged Rail Shot", { L(Lowpass(Tone(Waveform::Saw, 150.0f, 1200.0f, 0.3f, 0.05f, 0.3f, 0.15f, 0.1f, 0.8f), 5000.0f)), L(Burst(6000.0f, 400.0f, 0.03f, 0.4f, 0.8f), 0.32f), L(Thump(150.0f, 30.0f, 0.3f, 0.8f), 0.32f) });
			Add(effects, "Guns", "Flamethrower", { L(Burst(1500.0f, 900.0f, 0.4f, 0.15f, 0.5f, 0.05f, 0.8f)), L(Lowpass(Tone(Waveform::Saw, 55.0f, 55.0f, 0.4f, 0.15f, 0.25f, 0.05f, 0.1f, 0.8f), 400.0f)) });
			Add(effects, "Guns", "Rocket Launch", { L(Burst(3000.0f, 500.0f, 0.3f, 0.5f, 0.5f, 0.05f, 0.7f)), L(Thump(120.0f, 40.0f, 0.3f, 0.6f)) });
			Add(effects, "Guns", "Machine Gun Burst", {
				L(Burst(5000.0f, 1500.0f, 0.01f, 0.07f, 0.5f), 0.0f), L(Thump(200.0f, 70.0f, 0.06f, 0.5f), 0.0f),
				L(Burst(5000.0f, 1500.0f, 0.01f, 0.07f, 0.5f), 0.07f), L(Thump(200.0f, 70.0f, 0.06f, 0.5f), 0.07f),
				L(Burst(5000.0f, 1500.0f, 0.01f, 0.07f, 0.5f), 0.14f), L(Thump(200.0f, 70.0f, 0.06f, 0.5f), 0.14f),
				L(Burst(5000.0f, 1500.0f, 0.01f, 0.07f, 0.5f), 0.21f), L(Thump(200.0f, 70.0f, 0.06f, 0.5f), 0.21f)
			});
		}

		void AddBows(std::vector<SynthEffect>& effects)
		{
			Add(effects, "Bows", "Bow Draw", { L(Lowpass(Wobble(Tone(Waveform::Saw, 90.0f, 140.0f, 0.35f, 0.1f, 0.25f, 0.15f, 0.05f, 0.9f), 12.0f, 0.1f), 900.0f)), L(Burst(1200.0f, 1200.0f, 0.3f, 0.1f, 0.1f, 0.1f)) });
			Add(effects, "Bows", "Bow Release", { L(Burst(4000.0f, 800.0f, 0.01f, 0.1f, 0.35f)), L(Wobble(Tone(Waveform::Sine, 440.0f, 300.0f, 0.12f, 0.25f, 0.4f), 30.0f, 0.05f)), L(Burst(3000.0f, 6000.0f, 0.05f, 0.1f, 0.2f, 0.02f), 0.02f) });
			Add(effects, "Bows", "Arrow Whoosh", { L(Burst(800.0f, 3000.0f, 0.15f, 0.1f, 0.3f, 0.05f)) });
			Add(effects, "Bows", "Arrow Hit", { L(Thump(160.0f, 60.0f, 0.08f, 0.6f)), L(Burst(1800.0f, 600.0f, 0.01f, 0.06f, 0.3f)), L(Wobble(Tone(Waveform::Sine, 600.0f, 600.0f, 0.01f, 0.3f, 0.15f), 35.0f, 0.03f), 0.02f) });
			Add(effects, "Bows", "Crossbow", { L(Tone(Waveform::Square, 900.0f, 700.0f, 0.006f, 0.03f, 0.3f)), L(Burst(5000.0f, 1500.0f, 0.01f, 0.12f, 0.45f), 0.01f), L(Thump(250.0f, 90.0f, 0.1f, 0.5f), 0.01f) });
			Add(effects, "Bows", "Volley", {
				L(Burst(4000.0f, 800.0f, 0.01f, 0.1f, 0.3f), 0.0f), L(Burst(4000.0f, 800.0f, 0.01f, 0.1f, 0.3f), 0.04f),
				L(Burst(4000.0f, 800.0f, 0.01f, 0.1f, 0.3f), 0.09f), L(Burst(800.0f, 3000.0f, 0.2f, 0.15f, 0.25f, 0.06f), 0.05f)
			});
		}

		void AddMagic(std::vector<SynthEffect>& effects)
		{
			Add(effects, "Magic", "Fireball", { L(Burst(2000.0f, 500.0f, 0.2f, 0.3f, 0.5f, 0.03f)), L(Wobble(Tone(Waveform::Sine, 400.0f, 120.0f, 0.2f, 0.2f, 0.4f), 18.0f, 0.06f)) });
			Add(effects, "Magic", "Ice Shard", { L(Tone(Waveform::Triangle, 2800.0f, 1200.0f, 0.08f, 0.15f, 0.3f)), L(Burst(8000.0f, 3000.0f, 0.02f, 0.1f, 0.2f)) });
			Add(effects, "Magic", "Lightning Zap", { L(Lowpass(Tone(Waveform::Crunch, 2500.0f, 200.0f, 0.15f, 0.1f, 0.4f), 6000.0f, 1000.0f)), L(Burst(7000.0f, 1500.0f, 0.1f, 0.1f, 0.3f)) });
			Add(effects, "Magic", "Magic Bolt", { L(Lowpass(Tone(Waveform::Saw, 700.0f, 300.0f, 0.12f, 0.1f, 0.3f), 3500.0f, 1200.0f)), L(Tone(Waveform::Sine, 1400.0f, 500.0f, 0.1f, 0.08f, 0.25f)) });
			Add(effects, "Magic", "Heal", { L(Wobble(Tone(Waveform::Sine, 440.0f, 880.0f, 0.25f, 0.2f, 0.4f, 0.04f), 6.0f, 0.02f)), L(Tone(Waveform::Sine, 660.0f, 1320.0f, 0.2f, 0.2f, 0.25f, 0.04f), 0.08f), L(Tone(Waveform::Triangle, 1760.0f, 1760.0f, 0.05f, 0.2f, 0.15f), 0.2f) });
			Add(effects, "Magic", "Buff", { L(Tone(Waveform::Triangle, 330.0f, 660.0f, 0.2f, 0.15f, 0.4f, 0.02f)), L(Tone(Waveform::Triangle, 495.0f, 990.0f, 0.2f, 0.15f, 0.3f, 0.02f), 0.06f), L(Tone(Waveform::Sine, 1320.0f, 1320.0f, 0.04f, 0.2f, 0.2f), 0.2f) });
			Add(effects, "Magic", "Curse", { L(Wobble(Lowpass(Tone(Waveform::Saw, 300.0f, 80.0f, 0.35f, 0.15f, 0.35f, 0.02f, 0.1f, 0.8f), 1200.0f), 8.0f, 0.05f)), L(Tone(Waveform::Square, 150.0f, 100.0f, 0.3f, 0.1f, 0.15f)) });
			Add(effects, "Magic", "Shield Up", { L(Tone(Waveform::Sine, 300.0f, 900.0f, 0.2f, 0.15f, 0.4f, 0.05f)), L(Burst(1000.0f, 4000.0f, 0.2f, 0.1f, 0.15f, 0.05f)) });
			Add(effects, "Magic", "Teleport", { L(Wobble(Tone(Waveform::Sine, 200.0f, 2000.0f, 0.25f, 0.05f, 0.4f, 0.02f), 20.0f, 0.05f)), L(Wobble(Tone(Waveform::Sine, 2000.0f, 200.0f, 0.2f, 0.2f, 0.3f), 20.0f, 0.05f), 0.25f), L(Burst(500.0f, 6000.0f, 0.25f, 0.2f, 0.15f, 0.05f)) });
			Add(effects, "Magic", "Summon", { L(Lowpass(Tone(Waveform::Saw, 60.0f, 200.0f, 0.6f, 0.3f, 0.3f, 0.2f, 0.1f, 0.9f), 600.0f)), L(Tone(Waveform::Sine, 400.0f, 1200.0f, 0.5f, 0.3f, 0.25f, 0.3f)), L(Burst(300.0f, 3000.0f, 0.6f, 0.3f, 0.15f, 0.3f)) });
		}

		void AddMelee(std::vector<SynthEffect>& effects)
		{
			Add(effects, "Melee", "Sword Swing", { L(Burst(500.0f, 4000.0f, 0.1f, 0.1f, 0.4f, 0.04f)) });
			Add(effects, "Melee", "Sword Clash", { L(Burst(8000.0f, 3000.0f, 0.01f, 0.08f, 0.5f)), L(Tone(Waveform::Sine, 2230.0f, 2230.0f, 0.01f, 0.35f, 0.18f)), L(Tone(Waveform::Sine, 3470.0f, 3470.0f, 0.01f, 0.25f, 0.12f)), L(Tone(Waveform::Square, 1800.0f, 1800.0f, 0.01f, 0.3f, 0.1f)) });
			Add(effects, "Melee", "Axe Chop", { L(Burst(2500.0f, 400.0f, 0.03f, 0.15f, 0.6f)), L(Thump(130.0f, 45.0f, 0.15f, 0.8f)) });
			Add(effects, "Melee", "Spear Thrust", { L(Burst(1000.0f, 3500.0f, 0.07f, 0.05f, 0.3f, 0.03f)), L(Thump(200.0f, 80.0f, 0.06f, 0.4f), 0.07f) });
			Add(effects, "Melee", "Shield Block", { L(Thump(180.0f, 120.0f, 0.12f, 0.6f)), L(Burst(3000.0f, 1500.0f, 0.01f, 0.1f, 0.4f)), L(Tone(Waveform::Sine, 1400.0f, 1400.0f, 0.01f, 0.2f, 0.15f)) });
			Add(effects, "Melee", "Whip Crack", { L(Lowpass(Tone(Waveform::Crunch, 4000.0f, 800.0f, 0.01f, 0.08f, 0.5f), 9000.0f, 2000.0f)), L(Burst(8000.0f, 2000.0f, 0.005f, 0.08f, 0.4f)) });
			Add(effects, "Melee", "Hammer Smash", { L(Thump(100.0f, 30.0f, 0.3f, 0.9f)), L(Burst(2500.0f, 300.0f, 0.03f, 0.25f, 0.6f)) });
		}

		void AddHits(std::vector<SynthEffect>& effects)
		{
			Add(effects, "Hits", "Punch", { L(Burst(1500.0f, 300.0f, 0.01f, 0.1f, 0.6f)), L(Thump(140.0f, 50.0f, 0.12f, 0.7f)) });
			Add(effects, "Hits", "Heavy Hit", { L(Burst(1200.0f, 200.0f, 0.03f, 0.2f, 0.7f)), L(Thump(90.0f, 30.0f, 0.25f, 0.9f)) });
			Add(effects, "Hits", "Slash", { L(Burst(3500.0f, 700.0f, 0.03f, 0.15f, 0.5f)), L(Thump(200.0f, 90.0f, 0.08f, 0.4f)) });
			Add(effects, "Hits", "Critical Hit", { L(Burst(1500.0f, 300.0f, 0.01f, 0.1f, 0.6f)), L(Thump(140.0f, 50.0f, 0.12f, 0.7f)), L(Tone(Waveform::Sine, 1760.0f, 1760.0f, 0.01f, 0.3f, 0.3f), 0.02f), L(Tone(Waveform::Sine, 2349.0f, 2349.0f, 0.01f, 0.25f, 0.2f), 0.04f) });
			Add(effects, "Hits", "Projectile Hit", { L(Thump(300.0f, 100.0f, 0.06f, 0.5f)), L(Burst(2500.0f, 800.0f, 0.01f, 0.06f, 0.3f)) });
			Add(effects, "Hits", "Enemy Hit", { L(Pulse(Tone(Waveform::Square, 220.0f, 110.0f, 0.05f, 0.08f, 0.4f), 0.25f)), L(Burst(1500.0f, 500.0f, 0.01f, 0.08f, 0.3f)) });
			Add(effects, "Hits", "Player Hurt", { L(Pulse(Tone(Waveform::Square, 300.0f, 150.0f, 0.06f, 0.1f, 0.4f), 0.25f)), L(Burst(1200.0f, 400.0f, 0.02f, 0.08f, 0.3f)) });
		}

		void AddPlayer(std::vector<SynthEffect>& effects)
		{
			Layers levelUp;
			AddNotes(levelUp, Waveform::Triangle, { 523.0f, 659.0f, 784.0f, 1047.0f }, 0.0f, 0.09f, 0.08f, 0.15f, 0.45f, 0.25f);
			levelUp.push_back(L(Tone(Waveform::Sine, 2093.0f, 2093.0f, 0.2f, 0.4f, 0.15f), 0.27f));
			Add(effects, "Player", "Level Up", std::move(levelUp));

			Layers quest;
			AddNotes(quest, Waveform::Triangle, { 392.0f, 523.0f, 659.0f, 784.0f }, 0.0f, 0.12f, 0.1f, 0.15f, 0.45f, 0.4f);
			quest.push_back(L(Tone(Waveform::Sine, 392.0f, 392.0f, 0.5f, 0.4f, 0.25f), 0.36f));
			Add(effects, "Player", "Quest Complete", std::move(quest));

			Add(effects, "Player", "Achievement Bell", { L(Tone(Waveform::Sine, 1318.0f, 1318.0f, 0.02f, 1.0f, 0.4f, 0.001f, 0.05f, 0.5f)), L(Tone(Waveform::Sine, 2637.0f, 2637.0f, 0.02f, 0.7f, 0.2f)), L(Tone(Waveform::Sine, 3951.0f, 3951.0f, 0.02f, 0.5f, 0.1f)) });
			Add(effects, "Player", "Respawn", { L(Tone(Waveform::Sine, 200.0f, 800.0f, 0.3f, 0.2f, 0.4f, 0.1f)), L(Tone(Waveform::Triangle, 400.0f, 1600.0f, 0.3f, 0.2f, 0.2f, 0.1f), 0.1f), L(Tone(Waveform::Sine, 2093.0f, 2093.0f, 0.05f, 0.3f, 0.15f), 0.35f) });
			Add(effects, "Player", "Death", { L(Lowpass(Tone(Waveform::Saw, 400.0f, 40.0f, 0.6f, 0.4f, 0.4f, 0.01f, 0.2f, 0.7f), 3000.0f, 200.0f)), L(Burst(500.0f, 200.0f, 0.5f, 0.5f, 0.25f, 0.05f)) });

			Layers unlock;
			AddNotes(unlock, Waveform::Sine, { 660.0f, 880.0f, 1320.0f }, 0.0f, 0.07f, 0.05f, 0.2f, 0.4f, 0.1f);
			Add(effects, "Player", "Skill Unlock", std::move(unlock));
		}

		void AddPickups(std::vector<SynthEffect>& effects)
		{
			Add(effects, "Pickups", "Coin", { L(Pulse(Tone(Waveform::Square, 988.0f, 988.0f, 0.05f, 0.03f, 0.3f, 0.002f, 0.01f, 0.9f), 0.25f)), L(Pulse(Tone(Waveform::Square, 1319.0f, 1319.0f, 0.12f, 0.15f, 0.3f, 0.002f, 0.02f, 0.8f), 0.25f), 0.06f) });
			Add(effects, "Pickups", "Gold Pile", { L(Tone(Waveform::Sine, 1568.0f, 1568.0f, 0.02f, 0.12f, 0.3f)), L(Tone(Waveform::Sine, 2093.0f, 2093.0f, 0.02f, 0.12f, 0.3f), 0.04f), L(Tone(Waveform::Sine, 1760.0f, 1760.0f, 0.02f, 0.12f, 0.3f), 0.09f), L(Tone(Waveform::Sine, 2349.0f, 2349.0f, 0.02f, 0.14f, 0.3f), 0.13f) });
			Add(effects, "Pickups", "Item Pickup", { L(Tone(Waveform::Triangle, 660.0f, 1320.0f, 0.12f, 0.08f, 0.5f, 0.005f, 0.03f, 0.8f)) });
			Add(effects, "Pickups", "Potion Drink", { L(Tone(Waveform::Sine, 250.0f, 120.0f, 0.04f, 0.06f, 0.4f)), L(Tone(Waveform::Sine, 250.0f, 120.0f, 0.04f, 0.06f, 0.4f), 0.12f), L(Tone(Waveform::Sine, 250.0f, 120.0f, 0.04f, 0.06f, 0.4f), 0.24f), L(Burst(1500.0f, 800.0f, 0.3f, 0.1f, 0.08f, 0.02f), 0.0f) });
			Add(effects, "Pickups", "Key", { L(Tone(Waveform::Sine, 2000.0f, 2000.0f, 0.01f, 0.3f, 0.25f)), L(Tone(Waveform::Sine, 3000.0f, 3000.0f, 0.01f, 0.2f, 0.15f), 0.04f) });
			Add(effects, "Pickups", "Gem", { L(Tone(Waveform::Sine, 1760.0f, 1760.0f, 0.01f, 0.4f, 0.3f)), L(Tone(Waveform::Sine, 2349.0f, 2349.0f, 0.01f, 0.4f, 0.25f), 0.07f), L(Tone(Waveform::Sine, 2960.0f, 2960.0f, 0.01f, 0.5f, 0.2f), 0.14f) });
		}

		void AddInterface(std::vector<SynthEffect>& effects)
		{
			Add(effects, "UI", "Click", { L(Tone(Waveform::Sine, 1200.0f, 900.0f, 0.01f, 0.03f, 0.3f)) });
			Add(effects, "UI", "Hover", { L(Tone(Waveform::Sine, 1800.0f, 1800.0f, 0.005f, 0.02f, 0.2f)) });
			Add(effects, "UI", "Confirm", { L(Tone(Waveform::Triangle, 660.0f, 660.0f, 0.05f, 0.05f, 0.4f)), L(Tone(Waveform::Triangle, 880.0f, 880.0f, 0.08f, 0.1f, 0.4f), 0.07f) });
			Add(effects, "UI", "Cancel", { L(Tone(Waveform::Triangle, 880.0f, 880.0f, 0.05f, 0.05f, 0.4f)), L(Tone(Waveform::Triangle, 550.0f, 550.0f, 0.08f, 0.1f, 0.4f), 0.07f) });
			Add(effects, "UI", "Error", { L(Tone(Waveform::Square, 160.0f, 160.0f, 0.1f, 0.05f, 0.35f)), L(Tone(Waveform::Square, 160.0f, 160.0f, 0.1f, 0.05f, 0.35f), 0.14f) });
			Add(effects, "UI", "Open Menu", { L(Burst(600.0f, 2500.0f, 0.06f, 0.05f, 0.25f, 0.02f)), L(Tone(Waveform::Sine, 500.0f, 700.0f, 0.04f, 0.08f, 0.3f)) });
			Add(effects, "UI", "Close Menu", { L(Burst(2500.0f, 600.0f, 0.06f, 0.05f, 0.25f, 0.02f)), L(Tone(Waveform::Sine, 700.0f, 500.0f, 0.04f, 0.08f, 0.3f)) });
			Add(effects, "UI", "Equip Item", { L(Tone(Waveform::Sine, 2000.0f, 2000.0f, 0.005f, 0.15f, 0.25f)), L(Burst(4000.0f, 2000.0f, 0.005f, 0.05f, 0.3f)), L(Thump(200.0f, 100.0f, 0.04f, 0.3f)) });
			Add(effects, "UI", "Purchase", { L(Burst(6000.0f, 6000.0f, 0.01f, 0.03f, 0.2f)), L(Tone(Waveform::Sine, 1568.0f, 1568.0f, 0.02f, 0.15f, 0.35f)), L(Tone(Waveform::Sine, 2093.0f, 2093.0f, 0.02f, 0.3f, 0.35f), 0.08f) });
			Add(effects, "UI", "Notification", { L(Tone(Waveform::Sine, 880.0f, 880.0f, 0.05f, 0.2f, 0.35f)), L(Tone(Waveform::Sine, 1320.0f, 1320.0f, 0.06f, 0.3f, 0.35f), 0.1f) });
			Add(effects, "UI", "Typing Tick", { L(Tone(Waveform::Square, 2000.0f, 1500.0f, 0.002f, 0.015f, 0.2f)) });
		}

		void AddMovement(std::vector<SynthEffect>& effects)
		{
			Add(effects, "Movement", "Footstep Stone", { L(Burst(900.0f, 300.0f, 0.01f, 0.06f, 0.45f)), L(Thump(120.0f, 70.0f, 0.05f, 0.3f)) });
			Add(effects, "Movement", "Footstep Grass", { L(Burst(2500.0f, 900.0f, 0.01f, 0.07f, 0.25f, 0.005f)) });
			Add(effects, "Movement", "Footstep Wood", { L(Burst(1200.0f, 500.0f, 0.01f, 0.06f, 0.4f)), L(Thump(180.0f, 100.0f, 0.05f, 0.35f)) });
			Add(effects, "Movement", "Footstep Water", { L(Burst(1500.0f, 600.0f, 0.02f, 0.15f, 0.3f)), L(Tone(Waveform::Sine, 500.0f, 900.0f, 0.02f, 0.06f, 0.15f), 0.04f) });
			Add(effects, "Movement", "Jump", { L(Pulse(Tone(Waveform::Square, 200.0f, 600.0f, 0.1f, 0.05f, 0.3f), 0.25f)) });
			Add(effects, "Movement", "Land", { L(Thump(100.0f, 50.0f, 0.12f, 0.6f)), L(Burst(700.0f, 300.0f, 0.01f, 0.08f, 0.3f)) });
			Add(effects, "Movement", "Dash", { L(Burst(1000.0f, 5000.0f, 0.12f, 0.08f, 0.3f, 0.03f)) });
			Add(effects, "Movement", "Door Open", { L(Lowpass(Wobble(Tone(Waveform::Saw, 70.0f, 110.0f, 0.4f, 0.15f, 0.25f, 0.1f), 9.0f, 0.1f), 500.0f)), L(Burst(400.0f, 400.0f, 0.35f, 0.1f, 0.1f, 0.1f)) });
			Add(effects, "Movement", "Door Close", { L(Thump(100.0f, 40.0f, 0.15f, 0.7f)), L(Burst(800.0f, 300.0f, 0.02f, 0.1f, 0.3f)) });

			Layers chest = { L(Lowpass(Wobble(Tone(Waveform::Saw, 100.0f, 180.0f, 0.3f, 0.1f, 0.2f, 0.05f), 10.0f, 0.1f), 700.0f)) };
			AddNotes(chest, Waveform::Sine, { 1046.0f, 1318.0f, 1568.0f }, 0.3f, 0.06f, 0.04f, 0.25f, 0.3f, 0.08f);
			Add(effects, "Movement", "Chest Open", std::move(chest));

			Add(effects, "Movement", "Trap Trigger", { L(Tone(Waveform::Square, 900.0f, 500.0f, 0.005f, 0.04f, 0.3f)), L(Burst(2000.0f, 500.0f, 0.2f, 0.2f, 0.4f, 0.01f), 0.15f) });
		}

		void AddEnemies(std::vector<SynthEffect>& effects)
		{
			Add(effects, "Enemies", "Growl", { L(Lowpass(Wobble(Tone(Waveform::Saw, 90.0f, 60.0f, 0.5f, 0.2f, 0.5f, 0.05f, 0.1f, 0.9f), 25.0f, 0.08f), 700.0f)), L(Burst(600.0f, 300.0f, 0.4f, 0.2f, 0.2f, 0.05f)) });
			Add(effects, "Enemies", "Roar", { L(Lowpass(Wobble(Tone(Waveform::Saw, 160.0f, 70.0f, 0.7f, 0.4f, 0.5f, 0.05f, 0.1f, 0.9f), 20.0f, 0.1f), 1200.0f)), L(Burst(1500.0f, 400.0f, 0.6f, 0.3f, 0.3f, 0.05f)) });
			Add(effects, "Enemies", "Screech", { L(Lowpass(Wobble(Pulse(Tone(Waveform::Square, 3000.0f, 1800.0f, 0.2f, 0.1f, 0.3f), 0.2f), 40.0f, 0.1f), 5000.0f)) });
			Add(effects, "Enemies", "Slime Splat", { L(Tone(Waveform::Sine, 300.0f, 60.0f, 0.05f, 0.15f, 0.5f)), L(Burst(1500.0f, 400.0f, 0.02f, 0.12f, 0.35f)) });
			Add(effects, "Enemies", "Skeleton Rattle", {
				L(Lowpass(Tone(Waveform::Crunch, 900.0f, 900.0f, 0.015f, 0.03f, 0.4f), 4000.0f), 0.0f), L(Lowpass(Tone(Waveform::Crunch, 1100.0f, 1100.0f, 0.015f, 0.03f, 0.4f), 4000.0f), 0.05f),
				L(Lowpass(Tone(Waveform::Crunch, 800.0f, 800.0f, 0.015f, 0.03f, 0.4f), 4000.0f), 0.09f), L(Lowpass(Tone(Waveform::Crunch, 1000.0f, 1000.0f, 0.015f, 0.03f, 0.4f), 4000.0f), 0.15f),
				L(Lowpass(Tone(Waveform::Crunch, 950.0f, 950.0f, 0.015f, 0.03f, 0.4f), 4000.0f), 0.2f)
			});
			Add(effects, "Enemies", "Ghost Wail", { L(Wobble(Tone(Waveform::Sine, 500.0f, 700.0f, 0.6f, 0.4f, 0.3f, 0.3f), 5.0f, 0.05f)), L(Wobble(Tone(Waveform::Triangle, 750.0f, 1050.0f, 0.6f, 0.4f, 0.15f, 0.3f), 5.0f, 0.05f)) });
			Add(effects, "Enemies", "Boss Spawn", { L(Lowpass(Tone(Waveform::Saw, 40.0f, 90.0f, 1.0f, 0.5f, 0.5f, 0.5f, 0.1f, 0.9f), 500.0f, 2000.0f)), L(Burst(200.0f, 2500.0f, 1.0f, 0.5f, 0.35f, 0.6f)), L(Thump(60.0f, 30.0f, 0.8f, 0.9f), 1.0f) });
			Add(effects, "Enemies", "Enemy Death", { L(Lowpass(Tone(Waveform::Saw, 300.0f, 50.0f, 0.3f, 0.2f, 0.4f, 0.01f, 0.1f, 0.7f), 2500.0f, 300.0f)), L(Burst(1500.0f, 300.0f, 0.15f, 0.2f, 0.3f)) });
			Add(effects, "Enemies", "Wing Flap", { L(Burst(2000.0f, 500.0f, 0.04f, 0.06f, 0.3f)), L(Burst(2000.0f, 500.0f, 0.04f, 0.06f, 0.3f), 0.1f), L(Burst(2000.0f, 500.0f, 0.04f, 0.06f, 0.3f), 0.2f) });
		}

		void AddEnvironment(std::vector<SynthEffect>& effects)
		{
			Add(effects, "Environment", "Small Explosion", { L(Burst(3500.0f, 400.0f, 0.1f, 0.4f, 0.6f)), L(Thump(120.0f, 40.0f, 0.3f, 0.8f)) });
			Add(effects, "Environment", "Explosion", { L(Burst(3000.0f, 200.0f, 0.25f, 0.9f, 0.8f)), L(Thump(90.0f, 25.0f, 0.7f, 0.9f)) });
			Add(effects, "Environment", "Big Explosion", { L(Burst(2500.0f, 100.0f, 0.5f, 1.5f, 0.9f)), L(Thump(70.0f, 20.0f, 1.2f, 1.0f)) });
			Add(effects, "Environment", "Thunder", { L(Burst(700.0f, 150.0f, 0.8f, 1.4f, 0.7f, 0.1f)), L(Thump(50.0f, 25.0f, 1.0f, 0.6f), 0.1f) });
			Add(effects, "Environment", "Water Splash", { L(Burst(3000.0f, 800.0f, 0.1f, 0.25f, 0.45f)), L(Tone(Waveform::Sine, 400.0f, 800.0f, 0.06f, 0.1f, 0.2f), 0.05f) });
			Add(effects, "Environment", "Fire Crackle", {
				L(Lowpass(Tone(Waveform::Crunch, 1500.0f, 1500.0f, 0.01f, 0.03f, 0.4f), 5000.0f), 0.0f), L(Lowpass(Tone(Waveform::Crunch, 1800.0f, 1800.0f, 0.01f, 0.03f, 0.4f), 5000.0f), 0.07f),
				L(Lowpass(Tone(Waveform::Crunch, 1300.0f, 1300.0f, 0.01f, 0.03f, 0.4f), 5000.0f), 0.09f), L(Lowpass(Tone(Waveform::Crunch, 1700.0f, 1700.0f, 0.01f, 0.03f, 0.4f), 5000.0f), 0.2f),
				L(Lowpass(Tone(Waveform::Crunch, 1400.0f, 1400.0f, 0.01f, 0.03f, 0.4f), 5000.0f), 0.23f), L(Lowpass(Tone(Waveform::Crunch, 1600.0f, 1600.0f, 0.01f, 0.03f, 0.4f), 5000.0f), 0.32f)
			});
			Add(effects, "Environment", "Ice Shatter", { L(Tone(Waveform::Sine, 3520.0f, 3520.0f, 0.005f, 0.15f, 0.2f)), L(Tone(Waveform::Sine, 4186.0f, 4186.0f, 0.005f, 0.15f, 0.2f), 0.03f), L(Tone(Waveform::Sine, 3136.0f, 3136.0f, 0.005f, 0.15f, 0.2f), 0.06f), L(Tone(Waveform::Sine, 3951.0f, 3951.0f, 0.005f, 0.15f, 0.2f), 0.1f), L(Burst(8000.0f, 3000.0f, 0.02f, 0.15f, 0.3f)) });
			Add(effects, "Environment", "Wind Gust", { L(Burst(300.0f, 1800.0f, 0.5f, 0.5f, 0.25f, 0.3f)) });
			Add(effects, "Environment", "Rockslide", { L(Burst(500.0f, 200.0f, 0.6f, 0.5f, 0.5f, 0.1f)), L(Thump(60.0f, 30.0f, 0.6f, 0.5f), 0.1f) });
		}

		void AddAlerts(std::vector<SynthEffect>& effects)
		{
			Add(effects, "Alerts", "Warning Beeps", { L(Tone(Waveform::Square, 880.0f, 880.0f, 0.08f, 0.02f, 0.3f)), L(Tone(Waveform::Square, 880.0f, 880.0f, 0.08f, 0.02f, 0.3f), 0.15f), L(Tone(Waveform::Square, 880.0f, 880.0f, 0.08f, 0.02f, 0.3f), 0.3f) });
			Add(effects, "Alerts", "Siren", { L(Tone(Waveform::Square, 600.0f, 600.0f, 0.25f, 0.02f, 0.3f)), L(Tone(Waveform::Square, 800.0f, 800.0f, 0.25f, 0.02f, 0.3f), 0.28f) });
			Add(effects, "Alerts", "Danger Pulse", { L(Lowpass(Tone(Waveform::Saw, 110.0f, 110.0f, 0.2f, 0.1f, 0.4f), 800.0f)), L(Lowpass(Tone(Waveform::Saw, 110.0f, 110.0f, 0.2f, 0.1f, 0.4f), 800.0f), 0.4f) });
		}
	}

	const std::vector<SynthEffect>& SynthLibrary::GetEffects()
	{
		static const std::vector<SynthEffect> effects = []()
		{
			std::vector<SynthEffect> built;
			AddGuns(built);
			AddBows(built);
			AddMelee(built);
			AddHits(built);
			AddMagic(built);
			AddPlayer(built);
			AddPickups(built);
			AddInterface(built);
			AddMovement(built);
			AddEnemies(built);
			AddEnvironment(built);
			AddAlerts(built);
			return built;
		}();
		return effects;
	}

	const std::vector<std::string>& SynthLibrary::GetCategories()
	{
		static const std::vector<std::string> categories = []()
		{
			std::vector<std::string> built;
			for (const SynthEffect& effect : GetEffects()) {
				if (std::find(built.begin(), built.end(), effect.Category) == built.end()) {
					built.push_back(effect.Category);
				}
			}
			return built;
		}();
		return categories;
	}
}
