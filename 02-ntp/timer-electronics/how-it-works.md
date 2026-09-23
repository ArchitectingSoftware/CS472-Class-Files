# How LTspice Models a Crystal

LTspice can't simulate a vibrating piece of quartz directly, so it uses an equivalent circuit made of an inductor, a capacitor, a resistor, and a second capacitor. At the terminals, this circuit behaves just like the real crystal.

## Capacitors and Inductors as Frequency-Dependent Resistors

**A capacitor blocks low frequencies and passes high ones.** At DC it's an open circuit: charge piles up on the plates and current stops. As frequency rises, the plates never get a chance to fill before the current reverses, so it looks more and more like a wire.

**An inductor does the opposite: it passes low frequencies and blocks high ones.** At DC it's just a wire. As frequency rises, its magnetic field fights every change in current, and faster changes mean more opposition.

## The Series LC: Narrowing the Door

Put the inductor (Lm) and capacitor (Cm) in series, and current has to get through both:

- At **low** frequencies, the capacitor blocks it.
- At **high** frequencies, the inductor blocks it.
- At **exactly one** frequency, the two effects are equal and opposite, cancel out, and the branch looks like just the resistor (Rm).

That one frequency is **resonance**, the only frequency where the door is open. Everywhere else, the branch is nearly a dead end.

## Why This Behaves Like a Crystal

A quartz crystal is a tiny tuning fork that only wants to vibrate at one frequency, like a bell that only rings one note. Electrically, that looks exactly like the series LC:

| Model part | Physical meaning |
|---|---|
| Lm | The fork's mass (resists changes in motion) |
| Cm | The fork's springiness |
| Rm | Friction and energy loss |

The oscillator amplifies whatever noise is present, but only the resonant frequency makes it around the feedback loop. So that's the one that builds up and sustains.

## Getting It Started: The Kick

An oscillator doesn't create its frequency out of nothing. It needs a small disturbance to start from, and then the loop does the rest.

1. **Park the amplifier.** The feedback resistor (R1) biases the inverter at VS/2, where it acts as a linear amplifier instead of a switch. At this point everything is balanced and nothing is moving.
2. **Kick it.** A real circuit gets its kick for free from electrical noise and the power-up transient. A simulator has no noise, so a perfectly balanced circuit would sit at VS/2 forever. That's why the schematic includes `Ikick`, a brief current pulse into `xin`.
3. **The crystal picks the frequency.** A sharp pulse contains a little energy at every frequency. The amplifier boosts all of it, but the crystal's narrow door only lets the resonant frequency through, so everything else dies out.
4. **The loop grows it.** Each trip around the loop, the surviving frequency comes back amplified (loop gain above 1), so it grows cycle after cycle.
5. **It settles.** Eventually the signal gets large enough that the amplifier can't boost it any further. Gain drops to exactly 1, and the oscillation holds at a steady amplitude.

The kick only has to happen once. After that, the oscillator sustains itself.

## The Parallel Capacitor (C0)

C0 isn't part of the vibration at all. It's the real, physical capacitance between the crystal's two metal electrodes, which are two plates with quartz between them.

C0 sits **in parallel** with the series LC, so it acts as a side path around the narrow door. Two consequences:

- **It must stay small.** If C0 (plus any stray board capacitance) is too large compared to the load caps CL1/CL2, signal slips around the resonator instead of through it, and the oscillator won't start.
- **It shifts the frequency slightly.** C0 combines with the load capacitance to nudge the operating frequency a bit above the pure LC resonance. This is why a crystal's rated frequency is specified at a particular load capacitance.