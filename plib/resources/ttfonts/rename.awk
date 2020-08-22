BEGIN { 
sub( "-", "_", name ); 
}

/bin_data/ { print "char " name "_ttf_font[] = " }
!/bin_data/ { print $0 }

END {
print "unsigned int " name "_ttf_font_size = sizeof(" name "_ttf_font);" 
}