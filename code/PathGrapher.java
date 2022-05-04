import java.io.*;
import java.awt.*;
import javax.swing.*;
import java.awt.event.*;
import java.util.*;
import java.awt.geom.*;
import java.awt.image.BufferedImage;

public class PathGrapher extends JFrame {

    private String fileName = null;
    private int width = 740;
    private int height = 740;
    private float scaleX = 0.0f;
    private float scaleY = 0.0f;
    private int maxX = 0;
    private int maxY = 0;

    private ArrayList<Ellipse2D.Float> endPoints = null;
    private ArrayList<Line2D.Float> path = null;

    private static final int dotRadius = 2;
    private static final int pathSize = 2;
    private static final int boundaries = 50;
    private static final Color bgColor = Color.LIGHT_GRAY;
    private static final Color dotColor = Color.black;
    private static final Color pathColor = Color.blue;

    private BufferedImage bufImg;

    public void setFileName(String fileName) {
        this.fileName = fileName;
    }

    public void setDimensions(int width, int height) {
        this.width = width;
        this.height = height;
    }

    public void paint(Graphics g) {
        Graphics2D g2d = (Graphics2D)g;
        g2d.drawImage(bufImg, null, 0, 0);
    }

    public void paintPicture(Graphics g) {
        Graphics2D g2d = (Graphics2D)g;
        Iterator<Line2D.Float> i;
        System.out.println("painting...");
        g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING,
                             RenderingHints.VALUE_ANTIALIAS_ON);
        g2d.setColor(bgColor);
        g2d.fillRect(0, 0, width, height);
        g2d.setColor(pathColor);

        Line2D.Float segment;
        i = path.iterator();
        while (i.hasNext()) {
          segment = i.next(); 
          g2d.draw(segment);
        }

        g2d.setColor(dotColor);
        g2d.fill(endPoints.get(0));
        g2d.fill(endPoints.get(1));
    }

    public void go() {
        try {
            BufferedReader br = new BufferedReader(new FileReader(fileName));
            String line;
            String[] points;
            // first line unnecessary for graphing 
            br.readLine();

            // TODO: check these lines again
            scaleX = (width - 2.0f * boundaries) / this.width;
            scaleY = (height - 2.0f * boundaries) / this.height;

            // line with path 
            line = br.readLine().trim();
            points = line.split(" ");

            ArrayList<Line2D.Float> path = new ArrayList<Line2D.Float>();
            ArrayList<Ellipse2D.Float> endPoints = new ArrayList<Ellipse2D.Float>();
            
            String source = points[0];             

            int r0, c0, r1, c1;
            Float coordX0, coordY0, coordX1, coordY1;
            r1 = Character.getNumericValue(source.charAt(1));
            c1 = Character.getNumericValue(source.charAt(3));

            coordX1 = boundaries + scaleX * r1 - dotRadius;
            coordY1 = boundaries + scaleY * c1 - dotRadius;

            // add starting dot 
            Ellipse2D.Float dotStart = new Ellipse2D.Float(coordX1, coordY1, 2 * dotRadius, 2 * dotRadius);
            endPoints.add(dotStart);

            coordX1 += dotRadius;
            coordY1 += dotRadius;

            for(int i = 1; i < points.length; i++) {
              r0 = r1;
              c0 = c1;
              coordX0 = coordX1;
              coordY0 = coordY1;
              String point = points[i];

              r1 = Character.getNumericValue(point.charAt(1));
              c1 = Character.getNumericValue(point.charAt(3));
              coordX1 = boundaries + scaleX * r1;
              coordY1 = boundaries + scaleX * c1;
              Line2D.Float segment = new Line2D.Float(coordX0, coordY0, coordX1, coordY1);
              path.add(segment);
            }

            coordX1 -= dotRadius;
            coordY1 -= dotRadius;

            Ellipse2D.Float dotEnd = new Ellipse2D.Float(coordX1, coordY1, 2 * dotRadius, 2 * dotRadius);
            endPoints.add(dotEnd);
      
            this.endPoints = endPoints;
            this.path = path;
            this.setDimensions(width, height);
            this.setSize(width, height);
            this.setResizable(false);
            this.setBackground(bgColor);

            // compute image
            bufImg = new BufferedImage(width, height, BufferedImage.TYPE_INT_ARGB);
            paintPicture(bufImg.createGraphics());

            setVisible(true);

            addWindowListener(new WindowAdapter() {
                public void windowClosing(WindowEvent e) { System.exit(0); }
            });
        } catch (Exception exc) {
            exc.printStackTrace();
            System.err.println(exc.getMessage());
        }
    }

    // TODO: account to obstacles!!! 
    public static void main(String args[]) {
        PathGrapher g = new PathGrapher();
        if (args.length < 3) {
          System.out.println("usage: java PathGrapher [output_filename] [rows] [cols] \n Visualizes the path output of an execution of A*\n");
        } else {
          g.setFileName(args[0]);
          g.setDimensions(Integer.parseInt(args[1]), Integer.parseInt(args[2]));
        }

        g.go();
    }
}