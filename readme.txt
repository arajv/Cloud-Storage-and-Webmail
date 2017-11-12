run frontMaster  - ./frontMaster -p portnumber
run mainadmin - ./mainadmin -p portnumber

issues to fix:

give frontMaster a file as input to read all the front addrs. right now front_addr.txt is hardcoded

give mainamdin front end and back end server list as arguments. right now hardcoded both front_addr.txt and backend_addr.txt
need to make quit funcionality for front end in admin console
need to keep a link to main page of admin after going to shutdown page

for now the code ignores the first line in the address text files. so you can run assuming that first line will be taken as the address where frontMaster or mainadmin will run. 
