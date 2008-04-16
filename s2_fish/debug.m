mr = 4;
mphi = 4;
mz = 4;

rmax = 1.0;
deltar = rmax*2.0/(2.0*mr+1.0);

b = zeros(mr,mphi, mz);
r0 = 0.2;
x0 = r0/deltar + 0.5;

for i = 1:mr
  binleft = (i-1)*deltar;
  binright = i*deltar;
###  r = (i-0.5)*deltar;
  r = (binright-binleft)/2.0;
  if (binright < r0)
    b(i,:,:) = -4*pi;
  elseif (binleft < r0)
    b(i,:,:) = -4*pi*(r0**2-binleft**2)/(binright**2-binleft**2);
  else
    b(i,:,:) = 0.0;
  endif
###  printf("i=%d, r=%f, x0=%f\n",i,r,x0);
endfor

b

blm = zeros(mr, mphi, mz);
tmp = zeros(mphi,mz);
for i=1:mr
  tmp(:,:) = b(i,:,:);
  blm(i,:,:) = fft2(tmp);
endfor

A = zeros(mr,mr);
rsoln = zeros(mr,1);
psilm = zeros(mr,mphi, mz);
psi = zeros(mr, mphi, mz);

for l=0:mphi-1
  for m=0:mz-1
    for i=1:mr
      r = (i-0.5)*deltar;
      rsoln(i) = r;
      if (i == 1)
	A(i,i+1) = 1.0/deltar**2 + 1.0/(2*deltar*r) ;
      elseif (i==mr)
	A(i,i-1) = 1.0/deltar**2 - 1.0/(2*deltar*r);
      else
	A(i,i-1) = 1.0/deltar**2 - 1.0/(2*deltar*r);
	A(i,i+1) = 1.0/deltar**2 + 1.0/(2*deltar*r);
      endif
      A(i,i) = -2.0*(1.0/deltar**2);
      A(i,i) += - l**2/r**2 - m**2;
    endfor
    psilm(:,l+1,m+1) = A\blm(:,l+1,m+1);
  endfor
endfor

for i=1:mr
  tmp(:,:) = psilm(i,:,:);
  psi(i,:,:) = ifft2(tmp);
endfor

psi22 = zeros(mr,1);
psi22 = psi(:,1,1);

plot(rsoln,psi22,'1-*;jim;');

hold on
#x1 = 0:.05:1;
### r0 = 0.2, radius of charge
x1 = 0:.05:.2;
x2 = .2:.05:1;
#plot(x1,.25*pi*(3 - 4*x1.**2 + x1.**4),'3;exact;')
plot(x1,-pi*x1.*x1 + pi*.2*.2*(-2*log(.2)+1),'3;Exact Solution;')
plot(x2,-2*pi*.2*.2*log(x2)+2*pi*.2*.2*log(1),'3;;')
###load soln.dat

hold off
