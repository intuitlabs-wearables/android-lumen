package com.techcasita.jersey;

import org.apache.log4j.Logger;

import javax.net.ssl.HttpsURLConnection;
import javax.ws.rs.GET;
import javax.ws.rs.Path;
import javax.ws.rs.Produces;
import javax.ws.rs.QueryParam;
import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.URL;

/**
 * <code>Forward</code> receives an HTTP GET request and looks for 5 {@link String} parameters.
 * The parameter values will be plugged into a JSON template,
 * which is then send to the PNG D2D environment for further processing.
 * Basically, an HTTP GET request gets augmented and is forwarded as an HTTPS POST.
 *
 */
@Path("forward")
public class Forward {
    private static final String ADDRESS = "https://png.d2d.msg.intuit.com/api/v2/push";
    private static String TEMPLATE = "{\"senderId\":\"$SID$\",\"gcm\":{\"dry_run\":false,\"time_to_live\":1,\"groups\":[\"$GRP$\"],\"data\":{\"payload\":\"" +
            "{\\\"style\\\":\\\"BigTextStyle\\\",\\\"BigTextStyle\\\":{\\\"bigContentTitle\\\":\\\"$TITLE$\\\",\\\"bigText\\\":\\\"$TEXT$\\\"},\\\"smallIcon\\\":\\\"$ICON$\\\",\\\"background\\\":\\\"ic_lightbulb_$ICON$\\\"}\"}}}";


    @GET
    @Produces("text/plain")
    public String forward(
            @QueryParam("sid") final String sid,
            @QueryParam("grp") final String grp,
            @QueryParam("title") final String title,
            @QueryParam("text") final String text,
            @QueryParam("icon") final String icon) {
        Logger.getLogger(getClass()).info("Request received to send " + title + " to " + grp);

        String msg = TEMPLATE;
        msg = msg.replace("$SID$", sid);
        msg = msg.replace("$GRP$", grp);
        msg = msg.replace("$TITLE$", title);
        msg = msg.replace("$TEXT$", text);
        msg = msg.replace("$ICON$", icon);
        Logger.getLogger(getClass()).info("Request message: " + msg);

        String result;
        int responseCode = -1;
        HttpsURLConnection con;

        try {
            URL obj = new URL(ADDRESS);
            con = (HttpsURLConnection) obj.openConnection();

            //add request header
            con.setRequestMethod("POST");
            //con.setRequestProperty("User-Agent", USER_AGENT);
            //con.setRequestProperty("Accept-Language", "en-US,en;q=0.5");
            con.setRequestProperty("Content-Type", "application/json");

            // Send post request
            con.setDoOutput(true);
            DataOutputStream wr = new DataOutputStream(con.getOutputStream());
            wr.writeBytes(msg);
            wr.flush();
            wr.close();

            responseCode = con.getResponseCode();
            Logger.getLogger(getClass()).info("\nSending 'POST' request to URL : " + ADDRESS);
            Logger.getLogger(getClass()).info("Post data : " + msg);
            Logger.getLogger(getClass()).info("Response Code : " + responseCode);

            BufferedReader in = new BufferedReader(new InputStreamReader(con.getInputStream()));
            String inputLine;
            StringBuilder response = new StringBuilder();

            while ((inputLine = in.readLine()) != null) {
                response.append(inputLine);
            }
            in.close();
            result = response.toString();

        } catch (IOException e) {
            result = e.toString();
        }
        Logger.getLogger(getClass()).info(result);
        return String.valueOf(responseCode);
    }
}

