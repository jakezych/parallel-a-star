import java.io.*;
import java.awt.*;
import javax.swing.*;
import java.awt.event.*;
import java.util.*;
import java.awt.geom.*;
import java.awt.image.BufferedImage;

public class PathGrapher extends JFrame {

  private String inputFileName = null;
  private String outputFileName = null;
  private int width = 1000;
  private int height = 1000;
  private int gridRows;
  private int gridCols;

  private int[][] grid; 

  private static final int margin = 25;
  private static final int outlineWidth = 0;
  private static final Color obstacleColor = Color.black;
  private static final Color nodeColor = Color.LIGHT_GRAY;
  private static final Color outlineColor = Color.white;
  private static final Color pathColor = Color.blue;

  private BufferedImage bufImg;

  public void setInputFileName(String fileName) {
    this.inputFileName = fileName;
  }

  public void setOutputFileName(String fileName) {
    this.outputFileName = fileName;
  }

  public void setCanvasDimensions(int width, int height) {
    this.width = width;
    this.height = height;
  }

  public void setGridDimensions(int rows, int cols) {
    this.gridRows = rows;
    this.gridCols = cols;
  }

  public void initializeGrid(int dim) {
    this.grid = new int[dim][dim];
  }

  public void paint(Graphics g) {
    Graphics2D g2d = (Graphics2D)g;
    g2d.drawImage(bufImg, null, 0, 0);
  }

  public void draw() {
    bufImg = new BufferedImage(width, height, BufferedImage.TYPE_INT_ARGB);
    this.setCanvasDimensions(width, height);
    this.setSize(width, height);
    this.setResizable(false);

    paintGraph(bufImg.createGraphics());

    setVisible(true);

    addWindowListener(new WindowAdapter() {
        public void windowClosing(WindowEvent e) { System.exit(0); }
    });
  }

  public int[] getCoordinates(int i, int j) {
    int[] coords = new int[4];
    int scaledWidth = width - 2*margin;
    int scaledHeight = height - 2*margin;
    coords[0] = margin + (scaledWidth * j) / gridCols;
    coords[1] = margin + (scaledHeight * i) / gridRows;
    coords[2] = margin + (scaledWidth * (j + 1)) / gridCols;
    coords[3] = margin + (scaledHeight * (i + 1)) / gridRows;
    return coords;
  }

  public void paintGraph(Graphics g) {
    Graphics2D g2d = (Graphics2D)g;
    System.out.println("painting...");
    g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING,
                          RenderingHints.VALUE_ANTIALIAS_ON);
    g2d.setStroke(new BasicStroke(outlineWidth));
    g2d.setColor(Color.DARK_GRAY);
    g2d.fillRect(0, 0, width, height);
    for (int i = 0; i < gridRows; i++) {
      for (int j = 0; j < gridCols; j++) {
        int node = grid[i][j];
        int[] coords = getCoordinates(i, j);
        // choose color based on node type 
        if (node == 0) {
          g2d.setColor(obstacleColor);
        } else if (node == 1) {
          g2d.setColor(nodeColor);
        } else {
          g2d.setColor(pathColor);
        }
        // fill rectangle
        g2d.fillRect(coords[0], coords[1], coords[2] - coords[0], coords[3] - coords[1]);
        g2d.setColor(outlineColor);
        // draw outline
        g2d.drawRect(coords[0], coords[1], coords[2] - coords[0], coords[3] - coords[1]);
      }
    }
  }

  public void readGraph() {
    try {
      BufferedReader br = new BufferedReader(new FileReader(inputFileName));
      String line = br.readLine().trim(); 
      // read dimensions
      int dim = Integer.parseInt(line);

      setGridDimensions(dim, dim);
      initializeGrid(dim);
      for (int i = 0; i < dim; i++) {
        line = br.readLine().trim();
        String[] elems = line.split(" ");
        for (int j = 0; j < dim; j++) {
          int node = Character.getNumericValue(elems[j].charAt(0));
          this.grid[i][j] = node;
        }
      }
    } catch (Exception exc) {
      exc.printStackTrace();
      System.err.println(exc.getMessage());
    }
  }

  public void readPath() {
    try {
      BufferedReader br = new BufferedReader(new FileReader(outputFileName));
      String line;
      String[] points;
      // first line unnecessary for graphing 
      br.readLine();

      // line with path 
      line = br.readLine().trim();
      points = line.split(" ");

      for(int i = 0; i < points.length; i++) {
        // need to account for multiple digit rows and cols
        int commaIndex = points[i].indexOf(",");
        String rowString = points[i].substring(1, commaIndex);
        String colString = points[i].substring(commaIndex + 1, points[i].length() - 1);
        int row = Integer.parseInt(rowString);
        int col = Integer.parseInt(colString);
        // path is represented by 2s in the grid 
        this.grid[row][col] = 2;
      }

  } catch (Exception exc) {
      exc.printStackTrace();
      System.err.println(exc.getMessage());
    }
  }


  public static void main(String args[]) {
    PathGrapher g = new PathGrapher();
    if (args.length < 2) {
      System.out.println("usage: java PathGrapher [input_filename] [output_filename] \n Visualizes the path output of an execution of A*\n");
    } else {
      g.setInputFileName(args[0]);
      g.setOutputFileName(args[1]);
      if (args.length == 4) {
        g.setCanvasDimensions(Integer.parseInt(args[2]), Integer.parseInt(args[3]));
      }
    }
    g.readGraph();
    g.readPath();
    g.draw();
  }
}