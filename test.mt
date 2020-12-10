use "main.mtl";

fn main() 
  {  
  var colors = ["r", "g", "b", "y", "c","m"];
  var x = 0;
  while (x < 1000000) 
  {
    color(colors[x%len(colors)], "b");
    print x;
    print "";
    x = x + 1;    
  }         
}

main();  
