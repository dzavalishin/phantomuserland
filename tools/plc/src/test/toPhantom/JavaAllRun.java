package test.toPhantom;

public class JavaAllRun {

	public static void main(String[] args) {
		
		IPhantomPrinter p = new IPhantomPrinter() {			
			@Override
			public void print(String s) {
				System.out.println(s);
				
			}
		};

		AllRun ar = new AllRun();
		
		ar.setPrinter(p);
		ar.runAll();
	}

}
