from sampling import AnalogSignal, TimestampedADC
from typing import Literal, Callable, Union, Tuple, Iterable
import argparse
import math
import sys
import socket

WriterFunc = Callable[[TimestampedADC.Sample], None]
SocketConfig = Union[Tuple[int, Literal["tcp", "udp"]], None]


def make_writer(
    adc: TimestampedADC,
    binary: bool = False,
    sockconfig: SocketConfig = None,
) -> Tuple[WriterFunc, Iterable[socket.socket]]:

    assert isinstance(adc, TimestampedADC), "ADC must be an instance of ADC"
    assert isinstance(binary, bool), "Binary must be a boolean"
    if sockconfig is not None:
        assert isinstance(sockconfig, tuple), "Sockconfig must be a tuple"
        assert len(sockconfig) == 2, "Sockconfig must have 3 elements"
        assert isinstance(sockconfig[0], int), "Port must be an integer"
        assert isinstance(sockconfig[1], str), "Socktype must be a string"
        assert 0 <= sockconfig[0] <= 65535, "Port must be between 1 and 65535"
        assert sockconfig[1] in ["tcp", "udp"], "Socktype must be tcp or udp"

    if sockconfig is None:
        if binary:

            def write(sample: TimestampedADC.Sample) -> None:
                sys.stdout.buffer.write(bytes(sample))

        else:

            def write(sample: TimestampedADC.Sample) -> None:
                print(str(sample))

        return (lambda sample: write(sample)), []

    # If sockconfig is not None

    if binary:

        def tobytes(sample: TimestampedADC.Sample) -> bytes:
            return bytes(sample)

    else:

        def tobytes(sample: TimestampedADC.Sample) -> bytes:
            return str(sample).encode()

    (port, socktype) = sockconfig

    if socktype == "tcp":
        # make a tcp server
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.bind(("", port))
        sock.listen(1)
        print(f"Listening on port={port} using {socktype}", file=sys.stderr)
        print(f"Socket: {sock}", file=sys.stderr)

        conn, addr = sock.accept()
        print(f"Accepted connection from {addr}", file=sys.stderr)

        def write(sample: TimestampedADC.Sample) -> None:
            try:
                conn.sendall(tobytes(sample))
                print(f'Sent \n"{sample}"\n to sock={sock}, addr={addr}', file=sys.stderr)
            except OSError as e:
                print(f"Error during `sendall` on conn={conn}: {e}; addr={addr}", file=sys.stderr)

        return write, [sock, conn]

    else:  # socktype == "udp"
        # make a udp server

        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.bind(("", port))
        print(f"Listening on port={port} using {socktype}", file=sys.stderr)
        print(f"Socket: {sock}", file=sys.stderr)

        def write(sample: TimestampedADC.Sample) -> None:
            try:
                sock.sendto(tobytes(sample), ("", port))
                print(f'Sent \n"{sample}"\n to sock={sock}, port={port}', file=sys.stderr)
            except OSError as e:
                print(f"Error during `sendto` on sock={sock}, port={port}: {e}", file=sys.stderr)

        return write, [sock]


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run the ADC simulation")
    parser.add_argument("--no-gui", default=False, action="store_true", help="Run without GUI")
    parser.add_argument("--sampling-rate", "-r", type=int, default=1200, help="Sampling rate in Hz")
    parser.add_argument("--bits", "-b", type=int, default=8, help="ADC resolution in bits")
    parser.add_argument("--noise", "-z", type=float, default=0.25, help="Noise")
    parser.add_argument("--frequency", "-f", type=float, default=50, help="Signal frequency in Hz")
    parser.add_argument("--voltage", "-v", type=float, default=240, help="Maximum voltage in volts")
    parser.add_argument("--current", "-i", type=float, default=10, help="Maximum current in amperes")
    parser.add_argument("--phasediff", "-p", type=float, default=15, help="V-I phase difference in degrees")
    parser.add_argument("--binary", default=False, action="store_true", help="Send binary data")
    parser.add_argument("--port", "-n", type=int, default=12345, help="Port number of client socket")
    parser.add_argument(
        "--socktype",
        "-s",
        type=str,
        default="none",
        required=False,
        help="Socket type (tcp or udp or none)",
    )
    parser.add_help = True

    args = parser.parse_args()

    signals = [
        # VA
        AnalogSignal(
            frequency_hz=args.frequency,
            amplitude=args.voltage,
            phase_rad=math.radians(0),
        ),
        # VB
        AnalogSignal(
            frequency_hz=args.frequency,
            amplitude=args.voltage,
            phase_rad=math.radians(120),
        ),
        # VC
        AnalogSignal(
            frequency_hz=args.frequency,
            amplitude=args.voltage,
            phase_rad=math.radians(240),
        ),
        # IA
        AnalogSignal(
            frequency_hz=args.frequency,
            amplitude=args.voltage,  # Because the signal is fed to the ADC
            phase_rad=math.radians(0 + args.phasediff),
        ),
        # IB
        AnalogSignal(
            frequency_hz=args.frequency,
            amplitude=args.voltage,  # Because the signal is fed to the ADC
            phase_rad=math.radians(120 + args.phasediff),
        ),
        # IC
        AnalogSignal(
            frequency_hz=args.frequency,
            amplitude=args.voltage,  # Because the signal is fed to the ADC
            phase_rad=math.radians(240 + args.phasediff),
        ),
    ]
    adc = TimestampedADC(
        resolution_bits=args.bits,
        sampling_rate_hz=args.sampling_rate,
        reference_voltage_v=args.voltage,
        channels=list(zip(signals, [args.noise] * len(signals))),
    )

    sockconfig: SocketConfig = None
    if args.socktype in ["tcp", "udp"]:
        sockconfig = (int(args.port), str(args.socktype))

    write, socks = make_writer(adc, binary=args.binary, sockconfig=sockconfig)

    try:
        if not args.no_gui:
            # TODO: Run the GUI
            pass

        for sample in adc.stream():
            write(sample)

    except Exception as e:
        print(f"Error: {e}")

    finally:
        for sock in socks:
            sock.close()
