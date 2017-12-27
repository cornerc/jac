
BN InitBN(char *str){//大きい実数を作る。

	BN x;
	char str2[10]="";
	int i, cnt=0, idt=0, bottom=0;


	if(str[0]=='-'){
		x.sign=1;
		bottom=1;
	}else{
		x.sign=0;
	}

	for(i=strlen(str)-1; i>=bottom; i--){
		sprintf(str2, "%s%c", str2, str[i]);
		cnt++;
		if((cnt%9)==0){//９桁分格納したら
			Reverse(str2);//文字列の反転
			x.num[idt++]=atoi(str2);
			sprintf(str2, "%s", "");//初期化
		}
	}
	Reverse(str2);//９桁に満たない最大の桁領域
	x.num[idt]=atoi(str2);



	x.max = ((x.num[idt])==0) ? idt : idt+1;
	if(x.max==0){//文字列が"0"の時の例外処理
		x.max=1;
	}

	for(i=x.max;i<BNSIZE;i++){
		x.num[i]=0;
	}

	return x;
}

void Reverse(char *str){//文字列の反転
	char str2[10];//一時退避エリア
	int i, cnt=0;
	for(i=strlen(str)-1; i>=0; i--)
		str2[i]=str[cnt++];
	for(i=strlen(str)-1; i>=0; i--)
		str[i]=str2[i];
}

void ShowBN(BN a){

	int i=a.max;

	if(a.sign)
		printf("-");

	printf("%d ", a.num[--i]);
	while((--i)>=0){
		printf("%09d ", a.num[i]);
	}
	printf("\n");
}

void ShowAllBN(BN a){
	int i;

	for(i=BNSIZE;i>0;i--){
		printf("%09d ", a.num[i-1]);
	}
	printf("max:%d\n", a.max);
	printf("sign:%d\n", a.sign);
}

BN AddBN(BN a, BN b){

	int i, max, NextAdd=0;
	BN x = InitBN("0"), tmp;
	

	if(a.sign==b.sign){//同符号

		max = (a.max<b.max) ? b.max : a.max;//桁の小さい方に合わせる

		for(i=0; i<max; i++){

			x.num[i]=a.num[i]+b.num[i];
			
			if(NextAdd==1){//桁あふれ
				x.num[i]+=1;
				NextAdd=0;
			}
			if((x.num[i]/1000000000)>=1){
				x.num[i]%=1000000000;
				NextAdd=1;
			}
		}

		if(NextAdd==1){//同じ桁数での桁あふれ
			x.num[max]+=1;
			max++;
		}

		x.max=max;
		x.sign = a.sign;

		
	} else {//異符号

		if(a.max > b.max){//大きい方の符号に合わせる。
			x.sign = a.sign;
		}else if(a.max < b.max){
			x.sign = b.sign;
			tmp=a;a=b;b=tmp;
			
		}else{

			for(i=a.max;i>0;i--){
				if(a.num[i-1] > b.num[i-1]){
					x.sign = a.sign;
					break;
				}else if(a.num[i-1] < b.num[i-1]){
					x.sign = b.sign;
					tmp=a;a=b;b=tmp;
					break;
				}

			}
			if(i==0){
				return InitBN("0");
			}
			
		}

		for(i=0; i<BNSIZE; i++){

			if(a.num[i]>=b.num[i]){
				x.num[i]=a.num[i]-b.num[i];
			}else{
				a.num[i+1]--;
				x.num[i]=(a.num[i]+1000000000)-b.num[i];
			}
		}

		//最大桁を決める
		for(i=BNSIZE;i>0;i--){
			if(x.num[i-1]!=0){
				x.max=i;
				break;
			}
		}

	}
	return x;
}

BN SubBN(BN a, BN b){
	return AddBN(a, CngSignBN(b));
}

BN MulBN(BN a, BN b){

	BN x=InitBN("0"), tmp, bin;
	int i, sign;

	//符号を決める
	if((a.sign==1&&b.sign==1)||(a.sign==0&&b.sign==0)) sign=0;
	else sign=1;

	
	//大きい方を決める
	if(SameBN(AbsBN(b), MaxBN(AbsBN(a), AbsBN(b)))){
		tmp=a;a=b;b=tmp;
	}

	a=AbsBN(a);
	b=AbsBN(b);


	while(SameBN(b, InitBN("0"))==0){
		bin=InitBN("1");
		tmp=a;

		while(SameBN(b, MaxBN(b, AddBN(bin, bin)))){
			tmp = AddBN(tmp, tmp);
			bin = AddBN(bin, bin);
		}

		x=AddBN(x, tmp);
		b=SubBN(b, bin);

	}

	//最大桁を決める
	for(i=BNSIZE;i>0;i--){
		if(x.num[i-1]!=0){
			x.max=i;
			break;
		}
	}

	x.sign = sign;

	return x;
}

BN DivBN(BN a, BN b){

	BN x=InitBN("0"), tmp, bin;
	int i, sign;

	if(SameBN(InitBN("0"), b)) exit(-1);// ０で割った場合

	//符号を決める
	if((a.sign==1&&b.sign==1)||(a.sign==0&&b.sign==0)) sign=0;
	else sign=1;

	if(SameBN(AbsBN(a), AbsBN(b))){// 絶対値が同じ場合
		x=InitBN("1");
		x.sign = sign;
		return x;

	} else if(SameBN(AbsBN(b), MaxBN(AbsBN(a),AbsBN(b)))){// a<bの場合
		return InitBN("0");
	} 

	

	a=AbsBN(a);
	b=AbsBN(b);


	while(SameBN(a, MaxBN(a, b))){
		bin=InitBN("1");
		tmp=b;

		while(SameBN(a, MaxBN(a, AddBN(tmp, tmp)))){
			tmp = AddBN(tmp, tmp);
			bin = AddBN(bin, bin);
		}

		x=AddBN(x, bin);
		a=SubBN(a, tmp);

	}

	//最大桁を決める
	for(i=BNSIZE;i>0;i--){
		if(x.num[i-1]!=0){
			x.max=i;
			break;
		}
	}

	x.sign = sign;

	return x;

}

BN ModBN(BN a, BN b){

	BN x=InitBN("0"), tmp, bin;
	int i, sign;

	if(SameBN(InitBN("0"), b)) exit(-1);// ０で剰余った場合

	if(a.sign==1||b.sign==1){
		printf("負数の剰余は面倒です。");
		return InitBN("0");
	}

	if(SameBN(a, b)){// a==bの場合
		return InitBN("0");
	} 

	if(SameBN(b, MaxBN(a, b))){// a<bの場合
		return a;
	}

	while(SameBN(a, MaxBN(a, b))){
		bin=InitBN("1");
		tmp=b;

		while(SameBN(a, MaxBN(a, AddBN(tmp, tmp)))){
			tmp = AddBN(tmp, tmp);
			bin = AddBN(bin, bin);
		}

		a=SubBN(a, tmp);

	}
	x=a;

	//最大桁を決める
	for(i=BNSIZE;i>0;i--){
		if(x.num[i-1]!=0){
			x.max=i;
			break;
		}
	}

	x.sign = sign;

	return x;

}

BN CngSignBN(BN a){

	BN x=a;
	x.sign = (a.sign) ? 0 : 1;

	return x;
}

BN MaxBN(BN a, BN b){

	int i;

	if(a.sign < b.sign){
		return a;
	}else if(a.sign > b.sign){
		return b; 
	}else if(a.sign==0){//正同士

		if(a.max > b.max){
			return a;
		}else if(a.max < b.max){
			return b;
		}else{
			for(i=a.max;i>0;i--){
				if(a.num[i-1] > b.num[i-1]){
					return a;
				}else if(a.num[i-1] < b.num[i-1]){
					return b;
				}
			}
		}
	}else{//負同士
		if(a.max > b.max){
			return b;
		}else if(a.max < b.max){
			return a;
		}else{
			for(i=a.max;i>0;i--){
				if(a.num[i-1] > b.num[i-1]){
					return b;
				}else if(a.num[i-1] < b.num[i-1]){
					return a;
				}
			}
		}
	}

	return a;
}

BN MinBN(BN a, BN b){

	int i;

	if(a.sign < b.sign){
		return b;
	}else if(a.sign > b.sign){
		return a; 
	}else if(a.sign==0){//正同士

		if(a.max > b.max){
			return b;
		}else if(a.max < b.max){
			return a;
		}else{
			for(i=a.max;i>0;i--){
				if(a.num[i-1] > b.num[i-1]){
					return b;
				}else if(a.num[i-1] < b.num[i-1]){
					return a;
				}
			}
		}
	}else{//負同士
		if(a.max > b.max){
			return a;
		}else if(a.max < b.max){
			return b;
		}else{
			for(i=a.max;i>0;i--){
				if(a.num[i-1] > b.num[i-1]){
					return a;
				}else if(a.num[i-1] < b.num[i-1]){
					return b;
				}
			}
		}
	}

	return a;
}

int SameBN(BN a, BN b){
	int i;

	if((a.max == b.max) && (a.sign==b.sign)){
		for(i=a.max; i>0; i--){
			if(a.num[i-1] != b.num[i-1]){
				return 0;
			}
		}
		return 1;

	}else{
		return 0;
	}
}

BN AbsBN(BN a){
	a.sign=0;

	return a;
}