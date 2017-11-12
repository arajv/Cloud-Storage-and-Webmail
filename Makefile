all:
	protoc --cpp_out=. request.proto
	pkg-config --cflags protobuf
	g++ server_bck.cc process_operation.cc protobuf.cc process_req_bck.cc fault_tolerance.cc replication.cc utils.cc request.pb.cc -o server_bck `pkg-config --cflags --libs protobuf` 
	g++ server_front.cc request.pb.cc utils.cc protobuf.cc process_request_front.cc replication.cc smtp_client.cc -lresolv -o server_front `pkg-config --cflags --libs protobuf`
	g++ server_back_master.cc protobuf.cc request.pb.cc utils.cc -o server_back_master `pkg-config --cflags --libs protobuf` 
	g++ smtp.cc request.pb.cc protobuf.cc replication.cc utils.cc -o smtp `pkg-config --cflags --libs protobuf`
	g++ mainadmin.cpp request.pb.cc protobuf.cc -o mainadmin `pkg-config --cflags --libs protobuf`
	g++ frontMaster.cpp request.pb.cc protobuf.cc -o frontMaster `pkg-config --cflags --libs protobuf`

clean:
	rm -fv server_front server_bck server_back_master smtp request.pb.cc request.pb.h frontMaster mainadmin
	rm -r checkpoints/ logs/

pack:
	rm -f submit-project.zip
	zip -r submit-project.zip *.cpp *.cc *.hh *.h *.txt *.html README Makefile request.proto images/

