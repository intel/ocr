BEGIN{}
{if($2=="=") print "."$1$2$3; else print $0;}
END{}
