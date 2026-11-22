	while( num-- ) {

		if( ( abs( *S_data ) < thresh_value ) ||
			( attack_count < attack_limit ) ) 


 		{
		
			ratio = (float) attack_count / attack_limit;
			*S_data = (short) *S_data * ratio;
			attack_count++;
		}
		S_data++;

	}
