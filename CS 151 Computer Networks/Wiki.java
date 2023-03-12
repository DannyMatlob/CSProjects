package edu.sjsu.cs158a;

import picocli.CommandLine;
import picocli.CommandLine.Parameters;

import javax.swing.text.MutableAttributeSet;
import javax.swing.text.html.HTML;
import javax.swing.text.html.HTMLEditorKit;
import javax.swing.text.html.parser.ParserDelegator;
import java.io.IOException;
import java.io.InputStream;
import java.io.StringReader;
import java.net.URL;
import java.util.*;
import java.util.concurrent.Callable;

@CommandLine.Command
public class Wiki implements Callable<Integer> {
    String latestTitle = "";
    String targetPage = "Geographic_coordinate_system";
    HashSet<String> children = new LinkedHashSet<>();
    class wikiParser extends HTMLEditorKit.ParserCallback {
        final long startTime = System.currentTimeMillis();
        boolean isParent = false;
        private List<String> links = new ArrayList<String>();
        boolean titleFound = false;
        public wikiParser(boolean parent) {
            isParent = parent;
        }
        @Override
        public void handleStartTag(HTML.Tag t, MutableAttributeSet a, int pos) {
            if (t == HTML.Tag.TITLE) {
                titleFound = true;
            }

            if (t == HTML.Tag.A) {
                String href = (String) a.getAttribute(HTML.Attribute.HREF);
                if (href==null) return;
                if (href.startsWith("/wiki/")) {
                    String hrefSubject = href.substring(6);
                    if (hrefSubject.equals(targetPage)) {
                        System.out.println("Found in: " + latestTitle);
                        final long endTime = System.currentTimeMillis();
                        System.out.println("Total execution time: " + (endTime - startTime)/1000 + " seconds");
                        System.exit(0);
                    } else if (isParent && !hrefSubject.contains(":")) {
                        children.add(hrefSubject);
                    }
                }

            }
        }
        @Override
        public void handleText(char[] data, int pos) {
            if (titleFound) {
                latestTitle = new String(data);
                titleFound = false;
            }
            if (titleFound && isParent) System.out.println("Searching: " + new String(data));
        }
    }
    @Parameters(description = "Wikipedia Page to Search")
    String wikiPage;

    public String readPage(String wikiPage) {
        String wikiHTML = "";
        URL url = null;
        try {
            url = new URL("https://www.wikipedia.com/wiki/" + wikiPage);
            InputStream is = url.openStream();
            byte[] bytes = new byte[10000];
            int rc;
            while ((rc = is.read(bytes)) > 0) {
                wikiHTML += new String(bytes,0,rc);
            }
        } catch (IOException e) {
            System.out.println("Could not search: " + url);
            System.exit(1);
        }
        return wikiHTML;
    }

    public static void main(String[] args) {
        System.exit(new CommandLine(new Wiki()).execute(args));
    }

    public Integer call() throws IOException {
        ParserDelegator delegator = new ParserDelegator();

        //Parse parent page first
        delegator.parse(new StringReader(readPage(wikiPage)), new wikiParser(true), true);

        //Parse children until target is found
        System.out.println("Checking children:");
        var childParser = new wikiParser(false);
        for (String childPage : children) {
            var startTagTime1 = System.currentTimeMillis();
            //System.out.println("parsing child: " + childPage);
            delegator.parse(new StringReader(readPage(childPage)), childParser, true);
            var startTagTime2 = System.currentTimeMillis();
            System.out.println((startTagTime2 - startTagTime1) + " ms : "+ childPage);
        }

        //Final Return
        System.out.println("Not Found");

        return 0;
    }
}
