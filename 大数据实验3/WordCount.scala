import org.apache.spark.SparkContext
import org.apache.spark.SparkContext._
import org.apache.spark.SparkConf
import scala.collection.Map
object WordCount {
    def main(args: Array[String]) {
        val inputFile =  "test3.txt"
        val conf = new SparkConf().setMaster("local[2]").setAppName("WordCount")
	conf.set("spark.testing.memory", "500000000")
        val sc = new SparkContext(conf)
                val textFile = sc.textFile(inputFile)
                val wordCount = textFile.flatMap(line => line.split(" ")).map(word => (word, 1)).reduceByKey((a, b) => a + b)
                wordCount.foreach(println)
		wordCount.saveAsTextFile("outputtest3")       
    }
}