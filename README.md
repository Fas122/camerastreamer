Dependencies:
- ffmpeg
- opencv
- boost

Build:
cmake -S (source_dir) -B (build_dir)
cmake -S ../camerastreamer -B .

Run:
./media_server

View stream:
ffplay -fflags nobuffer -flags low_delay -i udp://127.0.0.1:5000
