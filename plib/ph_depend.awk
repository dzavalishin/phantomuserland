#BEGIN { sub( ".ph", ".pc", src); }

#! /@version@/ { print $0 }
  /import/ { 

	gsub( " ", "", $0 ); 
	sub( "import", "", $0 ); 
	sub( ";", ".pc", $0 ); 
	dep = $0

	if(substr(dep, 1, 2) == "//") 
		next

	if(substr(dep, 1, 1) == ".") 
    {
    	len = length(dep);
    	dep = substr(dep, 2, len-1 );
	}

	print src ": " dep; 
	next;
	}

#  { print $0 }
